
function note(id, text) {
  var el = document.getElementById(id);
  if (el) el.textContent = text;
}

async function sendRequest(method, path, options, noteId) {
  note(noteId, method + " " + path + " sent — check the webserv terminal.");
  try {
    await fetch(path, Object.assign({
      method: method,
      cache: "no-store"
    }, options || {}));
  } catch (e) {
    note(noteId, "Request sent. Check the terminal (browser reports a network error if the server closed the connection).");
  }
}

function sendRawUpload() {
  var body = document.getElementById("raw-body").value;
  sendRequest("POST", "/upload/browser-demo.txt", {
    headers: {"Content-Type": "text/plain"},
    body: body
  }, "raw-note");
}

function sendMultipart() {
  var input = document.getElementById("file-input");
  if (!input.files.length) {
    note("file-note", "Choose a file first.");
    return;
  }
  var data = new FormData();
  data.append("file", input.files[0]);
  sendRequest("POST", "/upload", {body: data}, "file-note");
}

function createDeleteFile() {
  sendRequest("POST", "/upload/delete-me.txt", {
    headers: {"Content-Type": "text/plain"},
    body: "delete me"
  }, "delete-note");
}

function deleteFile() {
  sendRequest("DELETE", "/uploads/delete-me.txt", {}, "delete-note");
}

function cgiGet() {
  var query = document.getElementById("cgi-query").value;
  sendRequest("GET", "/cgi/hello.py?" + query, {}, "cgi-get-note");
}

function cgiPost() {
  var body = document.getElementById("cgi-body").value;
  sendRequest("POST", "/cgi/env.py?from=browser", {
    headers: {"Content-Type": "text/plain"},
    body: body
  }, "cgi-post-note");
}

function triggerError(method, path, noteId) {
  sendRequest(method, path, method === "POST" ? {body: "x"} : {}, noteId);
}

function trigger413() {
  sendRequest("POST", "/upload/too-large.txt", {
    headers: {"Content-Type": "text/plain"},
    body: "X".repeat(2048)
  }, "error-note");
}

function trigger504() {
  sendRequest("GET", "/cgi/slow.py", {}, "error-note");
}
