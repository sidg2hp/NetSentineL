#!/bin/bash
# A simple automated test suite for the Proxy Server

PROXY_HOST="localhost"
PROXY_PORT="8888"

echo "--- STARTING PROXY TESTS ---"

# 1. Test a valid website (Example.com)
echo "[TEST 1] Accessing allowed site (http://example.com)..."
if curl -x $PROXY_HOST:$PROXY_PORT http://example.com | grep -q "Example Domain"; then
    echo "✅ PASS: Example.com content received."
else
    echo "❌ FAIL: Could not fetch Example.com"
fi

# 2. Test a blocked website
echo "[TEST 2] Accessing blocked site (http://badsite.com)..."
RESPONSE=$(curl -x $PROXY_HOST:$PROXY_PORT -s -o /dev/null -w "%{http_code}" http://badsite.com)
if [ "$RESPONSE" == "403" ]; then
    echo "✅ PASS: Badsite.com correctly returned 403 Forbidden."
else
    echo "❌ FAIL: Badsite.com returned $RESPONSE (Expected 403)"
fi

echo "--- TESTS COMPLETE ---"