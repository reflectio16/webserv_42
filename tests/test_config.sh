#!/bin/bash

WEBSERV="${1:-./webserv}"
TEST_DIR="$(mktemp -d /tmp/webserv_config_tests.XXXXXX)"

PASSED=0
FAILED=0

cleanup()
{
	rm -rf "$TEST_DIR"
}

trap cleanup EXIT

run_test()
{
	name="$1"
	expected="$2"
	file="$3"

	stdout_file="$TEST_DIR/stdout"
	stderr_file="$TEST_DIR/stderr"

	echo "------------------------------------------------------------"
	echo "TEST: $name"
	echo "Expected: $expected"

	"$WEBSERV" "$file" >"$stdout_file" 2>"$stderr_file"
	status=$?

	if [ "$status" -ge 128 ]; then
		echo "❌ CRASH (exit code $status)"
		echo "stdout:"
		cat "$stdout_file"
		echo "stderr:"
		cat "$stderr_file"
		FAILED=$((FAILED + 1))
		return
	fi

	if [ "$expected" = "PASS" ]; then
		if [ "$status" -eq 0 ]; then
			echo "✅ PASS"
			PASSED=$((PASSED + 1))
		else
			echo "❌ FAIL - valid config was rejected"
			echo "stderr:"
			cat "$stderr_file"
			FAILED=$((FAILED + 1))
		fi
	else
		if [ "$status" -ne 0 ]; then
			echo "✅ PASS - invalid config correctly rejected"

			if [ -s "$stderr_file" ]; then
				echo "Error:"
				cat "$stderr_file"
			fi

			PASSED=$((PASSED + 1))
		else
			echo "❌ FAIL - invalid config was accepted"

			if [ -s "$stdout_file" ]; then
				echo "stdout:"
				cat "$stdout_file"
			fi

			FAILED=$((FAILED + 1))
		fi
	fi
}

# ============================================================
# VALID CONFIGS
# ============================================================

cat > "$TEST_DIR/valid_minimal.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
}
EOF

run_test \
	"Valid minimal server" \
	"PASS" \
	"$TEST_DIR/valid_minimal.conf"


cat > "$TEST_DIR/valid_full.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	server_name example.com;
	root /var/www;

	client_max_body_size 1000000;

	error_page 404 /errors/404.html;
	error_page 500 /errors/500.html;

	location / {
		methods GET;
		index index.html;
		autoindex off;
	}

	location /images {
		methods GET;
		root /srv/images;
		index gallery.html;
		autoindex on;
	}

	location /upload {
		methods POST DELETE;
		upload_dir /var/www/uploads;
	}

	location /old {
		methods GET;
		redirect 301 /new;
	}

	location /cgi-bin {
		methods GET POST;
		root /var/www/cgi-bin;
		cgi_handler .py /usr/bin/python3;
	}
}
EOF

run_test \
	"Valid complete config" \
	"PASS" \
	"$TEST_DIR/valid_full.conf"


cat > "$TEST_DIR/valid_multiple_servers.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	server_name first.com;
	root /var/www/first;

	location / {
		methods GET;
	}
}

server {
	listen 127.0.0.1:9090;
	server_name second.com;
	root /var/www/second;

	location /api {
		methods GET POST;
	}
}
EOF

run_test \
	"Valid multiple servers" \
	"PASS" \
	"$TEST_DIR/valid_multiple_servers.conf"


cat > "$TEST_DIR/valid_virtual_hosts.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	server_name toto.com;
	root /var/www/toto;
}

server {
	listen 0.0.0.0:8080;
	server_name tata.com;
	root /var/www/tata;
}
EOF

run_test \
	"Valid virtual hosts on same endpoint" \
	"PASS" \
	"$TEST_DIR/valid_virtual_hosts.conf"


cat > "$TEST_DIR/valid_multiple_error_pages.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	error_page 400 /errors/400.html;
	error_page 404 /errors/404.html;
	error_page 500 /errors/500.html;
	error_page 503 /errors/503.html;
}
EOF

run_test \
	"Valid multiple error pages" \
	"PASS" \
	"$TEST_DIR/valid_multiple_error_pages.conf"


cat > "$TEST_DIR/valid_multiple_cgi.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /cgi {
		methods GET POST;
		cgi_handler .py /usr/bin/python3;
		cgi_handler .php /usr/bin/php-cgi;
	}
}
EOF

run_test \
	"Valid multiple CGI handlers" \
	"PASS" \
	"$TEST_DIR/valid_multiple_cgi.conf"


cat > "$TEST_DIR/valid_external_redirect.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /google {
		methods GET;
		redirect 302 https://google.com;
	}
}
EOF

run_test \
	"Valid external redirect" \
	"PASS" \
	"$TEST_DIR/valid_external_redirect.conf"


cat > "$TEST_DIR/all_one_line.conf" << 'EOF'
server { listen 0.0.0.0:8080; root /var/www; location / { methods GET POST; autoindex off; } }
EOF

run_test \
	"Entire config on one line" \
	"PASS" \
	"$TEST_DIR/all_one_line.conf"


cat > "$TEST_DIR/weird_whitespace.conf" << 'EOF'


server		{

	listen		0.0.0.0:8080	;

	root
	/var/www
	;

	location	/images	{

		methods		GET		;

		autoindex		on		;
	}

}

EOF

run_test \
	"Tabs and unusual whitespace" \
	"PASS" \
	"$TEST_DIR/weird_whitespace.conf"


cat > "$TEST_DIR/reordered.conf" << 'EOF'
server {
	error_page 404 /errors/404.html;

	location /upload {
		upload_dir /var/www/uploads;
		autoindex off;
		index index.html;
		methods POST;
	}

	client_max_body_size 10000;
	root /var/www;
	server_name example.com;
	listen 0.0.0.0:8080;
}
EOF

run_test \
	"Directives in unusual order" \
	"PASS" \
	"$TEST_DIR/reordered.conf"


# ============================================================
# SERVER ERRORS
# ============================================================

cat > "$TEST_DIR/missing_listen.conf" << 'EOF'
server {
	root /var/www;
}
EOF

run_test "Missing listen" "FAIL" "$TEST_DIR/missing_listen.conf"


cat > "$TEST_DIR/missing_root.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
}
EOF

run_test "Missing server root" "FAIL" "$TEST_DIR/missing_root.conf"


cat > "$TEST_DIR/duplicate_listen.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	listen 0.0.0.0:9090;
	root /var/www;
}
EOF

run_test "Duplicate listen" "FAIL" "$TEST_DIR/duplicate_listen.conf"


cat > "$TEST_DIR/duplicate_root.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
	root /srv/www;
}
EOF

run_test "Duplicate server root" "FAIL" "$TEST_DIR/duplicate_root.conf"


cat > "$TEST_DIR/unknown_server.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
	banana poulet;
}
EOF

run_test "Unknown server directive" "FAIL" "$TEST_DIR/unknown_server.conf"


# ============================================================
# LISTEN
# ============================================================

cat > "$TEST_DIR/port_zero.conf" << 'EOF'
server {
	listen 0.0.0.0:0;
	root /var/www;
}
EOF

run_test "Port zero" "FAIL" "$TEST_DIR/port_zero.conf"


cat > "$TEST_DIR/port_high.conf" << 'EOF'
server {
	listen 0.0.0.0:99999;
	root /var/www;
}
EOF

run_test "Port above 65535" "FAIL" "$TEST_DIR/port_high.conf"


cat > "$TEST_DIR/port_text.conf" << 'EOF'
server {
	listen 0.0.0.0:banana;
	root /var/www;
}
EOF

run_test "Non numeric port" "FAIL" "$TEST_DIR/port_text.conf"


cat > "$TEST_DIR/missing_host.conf" << 'EOF'
server {
	listen :8080;
	root /var/www;
}
EOF

run_test "Listen without host" "FAIL" "$TEST_DIR/missing_host.conf"


cat > "$TEST_DIR/missing_port.conf" << 'EOF'
server {
	listen 127.0.0.1:;
	root /var/www;
}
EOF

run_test "Listen without port" "FAIL" "$TEST_DIR/missing_port.conf"


cat > "$TEST_DIR/huge_port.conf" << 'EOF'
server {
	listen 0.0.0.0:999999999999999999999999999999999999999;
	root /var/www;
}
EOF

run_test "Huge overflowing port" "FAIL" "$TEST_DIR/huge_port.conf"


# ============================================================
# LOCATION / METHODS
# ============================================================

cat > "$TEST_DIR/location_without_methods.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /images {
		root /srv/images;
	}
}
EOF

run_test "Location without methods" "FAIL" "$TEST_DIR/location_without_methods.conf"


cat > "$TEST_DIR/duplicate_location.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /images {
		methods GET;
	}

	location /images {
		methods GET;
	}
}
EOF

run_test "Duplicate location path" "FAIL" "$TEST_DIR/duplicate_location.conf"


cat > "$TEST_DIR/unknown_location.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location / {
		methods GET;
		potato on;
	}
}
EOF

run_test "Unknown location directive" "FAIL" "$TEST_DIR/unknown_location.conf"


cat > "$TEST_DIR/invalid_method.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location / {
		methods GET BANANA;
	}
}
EOF

run_test "Invalid HTTP method" "FAIL" "$TEST_DIR/invalid_method.conf"


cat > "$TEST_DIR/duplicate_method.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location / {
		methods GET POST GET;
	}
}
EOF

run_test "Duplicate HTTP method" "FAIL" "$TEST_DIR/duplicate_method.conf"


cat > "$TEST_DIR/empty_methods.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location / {
		methods ;
	}
}
EOF

run_test "Empty methods" "FAIL" "$TEST_DIR/empty_methods.conf"


# ============================================================
# ROOT / INDEX / AUTOINDEX / UPLOAD
# ============================================================

cat > "$TEST_DIR/relative_root.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root var/www;
}
EOF

run_test "Relative server root" "FAIL" "$TEST_DIR/relative_root.conf"


cat > "$TEST_DIR/invalid_autoindex.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location / {
		methods GET;
		autoindex banana;
	}
}
EOF

run_test "Invalid autoindex" "FAIL" "$TEST_DIR/invalid_autoindex.conf"


cat > "$TEST_DIR/duplicate_autoindex.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location / {
		methods GET;
		autoindex on;
		autoindex off;
	}
}
EOF

run_test "Duplicate autoindex" "FAIL" "$TEST_DIR/duplicate_autoindex.conf"


cat > "$TEST_DIR/relative_upload.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /upload {
		methods POST;
		upload_dir uploads;
	}
}
EOF

run_test "Relative upload_dir" "FAIL" "$TEST_DIR/relative_upload.conf"


cat > "$TEST_DIR/duplicate_upload.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /upload {
		methods POST;
		upload_dir /uploads/a;
		upload_dir /uploads/b;
	}
}
EOF

run_test "Duplicate upload_dir" "FAIL" "$TEST_DIR/duplicate_upload.conf"


# ============================================================
# CLIENT MAX BODY SIZE
# ============================================================

cat > "$TEST_DIR/body_text.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
	client_max_body_size banana;
}
EOF

run_test "Invalid client_max_body_size" "FAIL" "$TEST_DIR/body_text.conf"


cat > "$TEST_DIR/body_negative.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
	client_max_body_size -100;
}
EOF

run_test "Negative client_max_body_size" "FAIL" "$TEST_DIR/body_negative.conf"


cat > "$TEST_DIR/body_duplicate.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
	client_max_body_size 100;
	client_max_body_size 200;
}
EOF

run_test "Duplicate client_max_body_size" "FAIL" "$TEST_DIR/body_duplicate.conf"


cat > "$TEST_DIR/body_huge.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
	client_max_body_size 999999999999999999999999999999999999999;
}
EOF

run_test "Huge client_max_body_size" "FAIL" "$TEST_DIR/body_huge.conf"


# ============================================================
# ERROR PAGES
# ============================================================

cat > "$TEST_DIR/error_200.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
	error_page 200 /errors/error.html;
}
EOF

run_test "Invalid error page status" "FAIL" "$TEST_DIR/error_200.conf"


cat > "$TEST_DIR/error_letters.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
	error_page 404abc /errors/error.html;
}
EOF

run_test "Error code containing letters" "FAIL" "$TEST_DIR/error_letters.conf"


cat > "$TEST_DIR/error_duplicate.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
	error_page 404 /errors/a.html;
	error_page 404 /errors/b.html;
}
EOF

run_test "Duplicate error_page" "FAIL" "$TEST_DIR/error_duplicate.conf"


# ============================================================
# REDIRECT
# ============================================================

cat > "$TEST_DIR/redirect_invalid.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /old {
		methods GET;
		redirect 404 /new;
	}
}
EOF

run_test "Invalid redirect code" "FAIL" "$TEST_DIR/redirect_invalid.conf"


cat > "$TEST_DIR/redirect_duplicate.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /old {
		methods GET;
		redirect 301 /new;
		redirect 302 /other;
	}
}
EOF

run_test "Duplicate redirect" "FAIL" "$TEST_DIR/redirect_duplicate.conf"


# ============================================================
# CGI
# ============================================================

cat > "$TEST_DIR/cgi_no_dot.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /cgi {
		methods GET;
		cgi_handler py /usr/bin/python3;
	}
}
EOF

run_test "CGI extension without dot" "FAIL" "$TEST_DIR/cgi_no_dot.conf"


cat > "$TEST_DIR/cgi_dot_only.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /cgi {
		methods GET;
		cgi_handler . /usr/bin/python3;
	}
}
EOF

run_test "CGI extension containing only dot" "FAIL" "$TEST_DIR/cgi_dot_only.conf"


cat > "$TEST_DIR/cgi_relative.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /cgi {
		methods GET;
		cgi_handler .py python3;
	}
}
EOF

run_test "Relative CGI interpreter" "FAIL" "$TEST_DIR/cgi_relative.conf"


cat > "$TEST_DIR/cgi_duplicate.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /cgi {
		methods GET;
		cgi_handler .py /usr/bin/python3;
		cgi_handler .py /usr/local/bin/python3;
	}
}
EOF

run_test "Duplicate CGI extension" "FAIL" "$TEST_DIR/cgi_duplicate.conf"


# ============================================================
# BROKEN SYNTAX / STRESS
# ============================================================

cat > "$TEST_DIR/empty.conf" << 'EOF'
EOF

run_test "Empty configuration" "FAIL" "$TEST_DIR/empty.conf"


cat > "$TEST_DIR/garbage.conf" << 'EOF'
banana {
	hello world;
}
EOF

run_test "Garbage at top level" "FAIL" "$TEST_DIR/garbage.conf"


cat > "$TEST_DIR/trailing_garbage.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
}

banana
EOF

run_test "Garbage after valid server" "FAIL" "$TEST_DIR/trailing_garbage.conf"


cat > "$TEST_DIR/extra_brace.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
}
}
EOF

run_test "Extra closing brace" "FAIL" "$TEST_DIR/extra_brace.conf"


cat > "$TEST_DIR/no_server_open.conf" << 'EOF'
server
	listen 0.0.0.0:8080;
	root /var/www;
}
EOF

run_test "Server without opening brace" "FAIL" "$TEST_DIR/no_server_open.conf"


cat > "$TEST_DIR/no_location_open.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location /images
		methods GET;
	}
}
EOF

run_test "Location without opening brace" "FAIL" "$TEST_DIR/no_location_open.conf"


cat > "$TEST_DIR/missing_semicolon.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www
}
EOF

run_test "Missing semicolon" "FAIL" "$TEST_DIR/missing_semicolon.conf"


cat > "$TEST_DIR/double_semicolon.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;;
	root /var/www;
}
EOF

run_test "Extra semicolon" "FAIL" "$TEST_DIR/double_semicolon.conf"


cat > "$TEST_DIR/unclosed_location.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;

	location / {
		methods GET;
}
EOF

run_test "Unclosed location/server" "FAIL" "$TEST_DIR/unclosed_location.conf"


cat > "$TEST_DIR/unclosed_server.conf" << 'EOF'
server {
	listen 0.0.0.0:8080;
	root /var/www;
EOF

run_test "Unclosed server" "FAIL" "$TEST_DIR/unclosed_server.conf"


# ============================================================
# RESULTS
# ============================================================

echo
echo "============================================================"
echo "CONFIG TEST RESULTS"
echo "============================================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo "Total : $((PASSED + FAILED))"
echo "============================================================"

if [ "$FAILED" -eq 0 ]; then
	echo "✅ ALL CONFIG TESTS PASSED"
	exit 0
else
	echo "❌ SOME CONFIG TESTS FAILED"
	exit 1
fi