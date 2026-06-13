#!/bin/bash

TEST_DIR="test"
EXE="./matching_engine_test"

# Color codes
RED='\033[0.31m'
GREEN='\033[0.32m'
NC='\033[0m'

if [ ! -f "$EXE" ]; then
    echo "Executable $EXE not found. Please run 'make' first."
    exit 1
fi

echo "--- STARTING EXTREME REGRESSION SUITE ---"

for test_file in $TEST_DIR/invalid_*.csv $TEST_DIR/absurd_*.csv; do
    echo "Testing: $test_file"
    
    $EXE "$test_file" > /dev/null 2> last_errors.log
    
    # Analyze if the parser crashed or behaved unexpectedly
    if [ $? -ne 0 ]; then
        echo -e "${RED}[CRASH]${NC} Parser crashed on $test_file"
    else
        ERROR_COUNT=$(grep -c "Error:" last_errors.log)
        VALID_LINES=$(grep -v "^#" "$test_file" | grep -v "^$" | wc -l)
        # In this specific test suite, almost all non-comment lines are designed to fail.
        # If ERROR_COUNT < VALID_LINES, it means some invalid lines were SILENTLY accepted (Potential Bug).
        if [ "$ERROR_COUNT" -lt "$VALID_LINES" ]; then
             echo -e "${RED}[WARNING]${NC} Potential silent failures in $test_file ($ERROR_COUNT errors logged, but $VALID_LINES invalid lines provided)"
        else
             echo -e "${GREEN}[OK]${NC} Parser handled all designed failures in $test_file"
        fi
    fi

    echo "----------------------------------------"
done

rm last_errors.log
echo "--- REGRESSION SUITE COMPLETE ---"
