#!/usr/bin/env bash

HOST="example.com"
URL="http://127.0.0.1:8080"

pass=0
fail=0

check_status()
{
	expected="$1"
	path="$2"
	method="$3"

	status=$(curl -s -o /dev/null \
		-w "%{http_code}" \
		-X "$method" \
		-H "Host: $HOST" \
		"$URL$path")

	if [ "$status" = "$expected" ]; then
		echo "PASS  $method $path -> $status"
		pass=$((pass + 1))
	else
		echo "FAIL  $method $path -> got $status, expected $expected"
		fail=$((fail + 1))
	fi
}

check_status 200 "/" GET
check_status 404 "/does-not-exist" GET
check_status 405 "/images/readme.txt" POST
check_status 501 "/images/readme.txt" PUT
check_status 301 "/old" GET
check_status 307 "/temp" GET
check_status 200 "/cgi/hello.py" GET

echo
echo "$pass passed, $fail failed"

if [ "$fail" -ne 0 ]; then
	exit 1
fi
