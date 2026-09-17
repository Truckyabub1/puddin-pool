#!/usr/bin/env bash
set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_ROOT="$(cd "${CLIENT_DIR}/.." && pwd)"

echo "================================================================="
echo "  PUDDIN'S POOL CI/CD VERIFICATION & AUTOMATED BUILD TEST"
echo "================================================================="

# 1. Run Headless WPA 8-Ball, 9-Ball & Physics Impact Dynamics Test Suites
echo -e "\n[STEP 1/3] Running Headless C++ & TypeScript Tournament Rules Tests..."
"${PROJECT_ROOT}/build/bin/nine_ball_rules_test"
"${PROJECT_ROOT}/build/bin/official_rules_test"
"${PROJECT_ROOT}/build/bin/physics_dynamics_test"
"${PROJECT_ROOT}/build/bin/test_impact_reactions"
"${PROJECT_ROOT}/build/bin/test_friction_decay"
cd "${CLIENT_DIR}"
npx tsx "${SCRIPT_DIR}/test_nine_ball_rules.ts"
echo "✓ All C++ & TypeScript Tournament Rules, Impact, & Friction Decay Tests Passed!"

# 2. Compile Web Client Production Bundle
echo -e "\n[STEP 2/3] Compiling TypeScript & Bundling via Vite..."
cd "${CLIENT_DIR}"
npm run build
echo "✓ Web Client Production Build Succeeded!"

# 3. Assert Production Bundle Size < 4.5MB Gzipped
echo -e "\n[STEP 3/3] Validating Production Bundle Size Budget (< 4.5MB gzipped)..."
DIST_DIR="${CLIENT_DIR}/dist"
if [ ! -d "${DIST_DIR}" ]; then
    echo "ERROR: ${DIST_DIR} not found!"
    exit 1
fi

TOTAL_BYTES=0
while IFS= read -r -d '' file; do
    BYTES=$(gzip -c "$file" | wc -c | tr -d ' ')
    TOTAL_BYTES=$((TOTAL_BYTES + BYTES))
done < <(find "${DIST_DIR}" -type f -print0)

TOTAL_KB=$((TOTAL_BYTES / 1024))
TOTAL_MB=$(awk "BEGIN {printf \"%.2f\", ${TOTAL_BYTES}/1048576}")
echo "Total Gzipped Dist Size: ${TOTAL_KB} KB (${TOTAL_MB} MB)"

# 4.5MB = 4718592 bytes
MAX_BYTES=4718592
if [ "${TOTAL_BYTES}" -le "${MAX_BYTES}" ]; then
    echo "✓ Bundle size check passed: ${TOTAL_MB}MB <= 4.50MB budget."
else
    echo "ERROR: Bundle size ${TOTAL_MB}MB exceeds 4.50MB budget!"
    exit 1
fi

echo -e "\n================================================================="
echo "  CI/CD VERIFICATION SUCCESSFUL (Exit Code: 0)"
echo "  READY FOR NETLIFY PRODUCTION DEPLOYMENT"
echo "================================================================="
exit 0
