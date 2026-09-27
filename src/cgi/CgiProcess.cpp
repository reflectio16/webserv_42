/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiProcess.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: meelma <meelma@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 21:56:03 by meelma            #+#    #+#             */
/*   Updated: 2026/09/27 23:08:23 by meelma           ###   ########.fr       */
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

    // Register the pipes in the loop. NOTE: these two calls are on the SERVER,
    // not here -- start() returns the fds/roles and the Server registers them,
    // OR start() takes a Server& / callback. See the integration note below.
    //   server.addCgiPipe(fromChild, conn.fd, POLLIN,  CGI_STDOUT);
    //   if (!conn.cgiStdin.empty())
    //       server.addCgiPipe(toChild, conn.fd, POLLOUT, CGI_STDIN);
    //   else
    //       { close(toChild); conn.cgiStdinFd = -1; }   // no body -> instant EOF

    return true;
}
    void onStdinWritable(Connection& conn)  { (void)conn; }
    void onStdoutReadable(Connection& conn) { (void)conn; }
    void cleanup(Connection& conn)          { (void)conn; }

}
