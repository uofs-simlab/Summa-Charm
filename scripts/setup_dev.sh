#!/bin/bash
# Development setup script - gets you working quickly

set -e

echo "🚀 Setting up SUMMA-Charm++ for development..."

# Step 1: Build working components
echo "Step 1: Building working core components..."
cd /u1/pma753/summa-charm
./scripts/clean.sh

mkdir -p build_dev
cd build_dev

cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DSUMMA_MINIMAL_BUILD=ON \
    -DBUILD_TESTS=ON

echo "Building core libraries..."
make summa_core summa_utils

echo "✅ Core libraries built successfully!"
ls -la lib/

# Step 2: Set up development environment
echo ""
echo "Step 2: Setting up development environment..."

# Create development configuration
cat > dev_config.json << 'EOF'
{
  "build_mode": "development",
  "summa_integration": "minimal",
  "charm_integration": "pending_fix",
  "working_components": [
    "libsumma_core.a",
    "libsumma_utils.a"
  ],
  "next_steps": [
    "Fix Charm++ CkMarshall.decl.h issue",
    "Complete SUMMA Fortran integration",
    "Build main executable"
  ]
}
EOF

# Create useful aliases
cat > dev_aliases.sh << 'EOF'
#!/bin/bash
# Development aliases for SUMMA-Charm++

alias sc-clean="cd /u1/pma753/summa-charm && ./scripts/clean.sh"
alias sc-build="cd /u1/pma753/summa-charm && mkdir -p build_dev && cd build_dev && cmake .. -DSUMMA_MINIMAL_BUILD=ON && make summa_core summa_utils"
alias sc-test="cd /u1/pma753/summa-charm/build_dev && ls lib/ && echo 'Libraries ready for development!'"
alias sc-status="cd /u1/pma753/summa-charm && echo 'Project status:' && ls -la build_dev/lib/ 2>/dev/null || echo 'Run sc-build first'"
EOF

chmod +x dev_aliases.sh

echo "✅ Development environment ready!"
echo ""
echo "📋 What you can do now:"
echo "• Your libraries: $(pwd)/lib/"
echo "• Configuration: $(pwd)/dev_config.json"
echo "• Aliases: source $(pwd)/dev_aliases.sh"
echo ""
echo "🔧 Quick commands:"
echo "• Clean & rebuild: sc-build"
echo "• Check status: sc-status"
echo "• Clean all: sc-clean"
echo ""
echo "🎯 Next development steps:"
echo "1. Fix Charm++ missing headers (rebuild Charm++ or find CkMarshall.decl.h)"
echo "2. Complete Fortran interface integration"
echo "3. Build and test main executable"
