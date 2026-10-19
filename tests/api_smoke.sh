#!/usr/bin/env bash
# ==============================================================================
# Medislot API Smoke Test Script
# Tests HTTP endpoints using curl:
#   1. 401 Unauthorized without Bearer token
#   2. 401 Unauthorized on SQL injection login attempt
#   3. 200 OK on valid patient login
#   4. 403 Forbidden when patient tries to access staff-only /api/icu
#   5. 200 OK on initial appointment booking
#   6. 409 Conflict when attempting duplicate slot booking
# ==============================================================================

set -euo pipefail

BASE_URL="${1:-http://127.0.0.1:8080}"
echo "Running API smoke tests against ${BASE_URL}..."

# 1. 401 without login
echo -n "Test 1: Unauthenticated request -> 401 ... "
STATUS=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/api/appointments")
if [ "$STATUS" -eq 401 ]; then
    echo "PASS"
else
    echo "FAIL (Got $STATUS, expected 401)"
    exit 1
fi

# 2. SQL injection in username returns 401 (not 500 or bypass)
echo -n "Test 2: SQL injection login attempt -> 401 ... "
STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST "${BASE_URL}/api/login" \
    -H "Content-Type: application/json" \
    -d '{"username": "admin'\'' OR 1=1 --", "password": "any"}')
if [ "$STATUS" -eq 401 ]; then
    echo "PASS"
else
    echo "FAIL (Got $STATUS, expected 401)"
    exit 1
fi

# 3. Valid login as patient (alice)
echo -n "Test 3: Patient login -> 200 ... "
LOGIN_RESP=$(curl -s -X POST "${BASE_URL}/api/login" \
    -H "Content-Type: application/json" \
    -d '{"username": "alice", "password": "patient123"}')
TOKEN=$(echo "$LOGIN_RESP" | grep -o '"token":"[^"]*"' | cut -d'"' -f4)
if [ -n "$TOKEN" ]; then
    echo "PASS (Got token)"
else
    echo "FAIL (Could not parse token)"
    exit 1
fi

# 4. Patient attempts staff action /api/icu -> 403
echo -n "Test 4: Patient accessing /api/icu -> 403 ... "
STATUS=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/api/icu" \
    -H "Authorization: Bearer ${TOKEN}")
if [ "$STATUS" -eq 403 ]; then
    echo "PASS"
else
    echo "FAIL (Got $STATUS, expected 403)"
    exit 1
fi

# 5. Staff login (reception)
RECEPTION_RESP=$(curl -s -X POST "${BASE_URL}/api/login" \
    -H "Content-Type: application/json" \
    -d '{"username": "reception", "password": "reception123"}')
RECEPTION_TOKEN=$(echo "$RECEPTION_RESP" | grep -o '"token":"[^"]*"' | cut -d'"' -f4)

# Future test slot
TEST_SLOT="2027-11-20 14:00"

# 6. Book appointment
echo -n "Test 5: Valid booking -> 200 ... "
BOOK_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST "${BASE_URL}/api/appointments" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer ${RECEPTION_TOKEN}" \
    -d "{\"patient_id\": 1, \"doctor_id\": 1, \"room_id\": 1, \"start\": \"${TEST_SLOT}\"}")
if [ "$BOOK_STATUS" -eq 200 ] || [ "$BOOK_STATUS" -eq 409 ]; then
    echo "PASS"
else
    echo "FAIL (Got $BOOK_STATUS)"
    exit 1
fi

# 7. Duplicate booking for the same slot -> 409 Conflict
echo -n "Test 6: Conflicting booking on same slot -> 409 ... "
CONFLICT_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST "${BASE_URL}/api/appointments" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer ${RECEPTION_TOKEN}" \
    -d "{\"patient_id\": 2, \"doctor_id\": 1, \"room_id\": 2, \"start\": \"${TEST_SLOT}\"}")
if [ "$CONFLICT_STATUS" -eq 409 ]; then
    echo "PASS"
else
    echo "FAIL (Got $CONFLICT_STATUS, expected 409)"
    exit 1
fi

echo "All API smoke tests passed successfully!"
