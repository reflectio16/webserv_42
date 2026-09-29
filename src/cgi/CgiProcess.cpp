/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiProcess.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 21:56:03 by meelma            #+#    #+#             */
/*   Updated: 2026/09/29 15:32:20 by meelma           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiProcess.hpp"
#include <unistd.h>     // fork, execve, dup2, close, chdir, pipe, _exit
#include <fcntl.h>      // fcntl, F_SETFL, F_SETFD, O_NONBLOCK, FD_CLOEXEC
#include <sys/time.h>   // (or however you get "now" for cgiStartMs)
#include <poll.h>       // POLLIN, POLLOUT
#include <cstddef>
#include <string>
#include <vector>
#include <iostream>
#include <sys/wait.h>   // waitpid, WNOHANG
#include <signal.h>     // kill, SIGKILL
//   #include <cstring>      // (if needed)


// Split "/var/www/cgi-bin/hello.py" -> dir "/var/www/cgi-bin", base "hello.py".
static void splitPath(const std::string& full, std::string& dir, std::string& base) {
    std::string::size_type slash = full.find_last_of('/');
    if (slash == std::string::npos) {   // no directory part
        dir  = ".";
        base = full;
    } else {
        dir  = full.substr(0, slash);
        base = full.substr(slash + 1);
    }
}

// Build a NULL-terminated char* array from a vector<string>. The returned
// vector<char*> points INTO the strings, so keep the source strings alive
// until after execve. Last element is NULL, as execve requires.
static std::vector<char*> toCArray(std::vector<std::string>& v) {
    std::vector<char*> out;
    for (size_t i = 0; i < v.size(); ++i)
        out.push_back(const_cast<char*>(v[i].c_str()));
    out.push_back(NULL);
    return out;
}

namespace CgiProcess {
    
bool start(Connection& conn, const Outcome& recipe) {
    int inPipe[2];    // body: parent writes inPipe[1] -> child reads inPipe[0] (stdin)
    int outPipe[2];   // output: child writes outPipe[1] (stdout) -> parent reads outPipe[0]

    if (pipe(inPipe) < 0)
        return false;
    if (pipe(outPipe) < 0) {
        close(inPipe[0]); close(inPipe[1]);
        return false;
    }

    // Split the script path so the child can chdir into its folder and exec
    // the basename (done in the PARENT -- pure string work, no chdir here).
    std::string dir, base;
    splitPath(recipe.cgiScriptPath, dir, base);

    pid_t pid = fork();
    if (pid < 0) {                       // fork failed -- clean up all four fds
        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);
        return false;
    }

    // ---------------- CHILD ----------------
    if (pid == 0) {
        // Wire the pipes onto stdin/stdout. dup2 also CLEARS FD_CLOEXEC on the
        // new fd, so 0 and 1 survive execve even though the originals won't.
        dup2(inPipe[0], 0);              // child's stdin  <- body pipe read end
        dup2(outPipe[1], 1);             // child's stdout -> output pipe write end

        // Close the four original pipe fds (we've dup'd the two we need).
        close(inPipe[0]);  close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);

        // Run from the script's own directory (CGI convention). CHILD ONLY --
        // never in the parent, or you'd move the whole server's cwd.
        if (chdir(dir.c_str()) < 0)
            _exit(1);

        // Build argv = { interpreter, basename, NULL } (or { basename, NULL }
        // if there's no interpreter and the script is directly executable).
        std::vector<std::string> argvStr;
        if (!recipe.cgiInterpreter.empty())
            argvStr.push_back(recipe.cgiInterpreter);
        argvStr.push_back(base);

        std::vector<std::string> envStr = recipe.cgiEnv;   // copy so c_str() stays valid
        std::vector<char*> argv = toCArray(argvStr);
        std::vector<char*> envp = toCArray(envStr);

        const char* path = recipe.cgiInterpreter.empty()
                           ? base.c_str()
                           : recipe.cgiInterpreter.c_str();

        execve(path, &argv[0], &envp[0]);
        _exit(1);                        // only reached if execve FAILED
    }

    // ---------------- PARENT ----------------
    // Close the ends the child owns; keep our two.
    close(inPipe[0]);    // child's stdin read end
    close(outPipe[1]);   // child's stdout write end

    int toChild   = inPipe[1];    // we WRITE the body here
    int fromChild = outPipe[0];   // we READ output here

    // Non-blocking, so feeding/draining never stalls the one thread.
    fcntl(toChild,   F_SETFL, O_NONBLOCK);
    fcntl(fromChild, F_SETFL, O_NONBLOCK);

    // Stamp the CGI state onto the connection.
    conn.cgiPid        = pid;
    conn.cgiStdinFd    = toChild;
    conn.cgiStdoutFd   = fromChild;
    conn.cgiStdin      = recipe.cgiBody;
    conn.cgiStdinOffset = 0;
    conn.cgiBuf.clear();
    // conn.cgiStartMs = nowMs();   // for the timeout (layer 4)
    conn.state         = CGI_RUNNING;

    if (conn.cgiStdin.empty()) {
        close(conn.cgiStdinFd);      // no body -> child reads instant EOF
        conn.cgiStdinFd = -1;        // signal "no stdin pipe to register"
    }

    std::cerr << "CGI forked, pid " << pid << "\n";   // TEMP debug -- remove later!!!
    return true;
}

void onStdinWritable(Connection& conn) {
    std::size_t remaining = conn.cgiStdin.size() - conn.cgiStdinOffset;
    ssize_t n = write(conn.cgiStdinFd,
                      conn.cgiStdin.data() + conn.cgiStdinOffset,
                      remaining);
 
    if (n < 0) {
        // child gone / pipe broke -> stop feeding; close the write end.
        // (SIGPIPE is ignored process-wide, so write just returns -1 here.)
        conn.cgiDoneWritingStdin = true;   // signal: unregister + close (Server side)
        return;
    }
    conn.cgiStdinOffset += static_cast<std::size_t>(n);
 
    if (conn.cgiStdinOffset >= conn.cgiStdin.size())
        conn.cgiDoneWritingStdin = true;   // whole body sent -> close stdin (Server side)
}
 
// Drain the child's stdout into cgiBuf. On EOF (read returns 0), the script is
// done: reap it, and the response is ready to be finalized.
void onStdoutReadable(Connection& conn) {
    char buf[4096];
    ssize_t n = read(conn.cgiStdoutFd, buf, sizeof(buf));
 
    if (n > 0) {
        conn.cgiBuf.append(buf, static_cast<std::size_t>(n));
        return;                            // more may come; wait for next POLLIN
    }
 
    // n == 0 (EOF) or n < 0 (error) -> the child is finished producing output.
    conn.cgiOutputComplete = true;         // signal: reap + finalize (Server side)
}
 
// Kill (if still alive) and reap the child, close the pipes. Called on normal
// completion, on timeout, and from closeConnection if a CGI was mid-flight.
void cleanup(Connection& conn) {
    if (conn.cgiPid > 0) {
        int status;
        // Reap without blocking; if it hasn't exited, kill it then reap.
        if (waitpid(conn.cgiPid, &status, WNOHANG) == 0) {
            kill(conn.cgiPid, SIGKILL);
            waitpid(conn.cgiPid, &status, 0);
        }
        conn.cgiPid = -1;
    }
    if (conn.cgiStdinFd != -1) {
        close(conn.cgiStdinFd);
        conn.cgiStdinFd = -1;
    }
    if (conn.cgiStdoutFd != -1) {
        close(conn.cgiStdoutFd);
        conn.cgiStdoutFd = -1;
    }
}


}
