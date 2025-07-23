import xarray as xr
import numpy as np

# Load NetCDF files
file1 = "/u1/pma753/SummaChare/test/output/bench/SummaOriginal__G000011-000020_timestep.nc"
file2 = "/u1/pma753/SummaChare/test/output/test/SummaOriginal__G000011-000020_timestep.nc"

ds1 = xr.open_dataset(file1)
ds2 = xr.open_dataset(file2)

# Compare dimensions
if ds1.dims != ds2.dims:
    print("❌ The dimensions differ.")
else:
    print("✅ Dimensions are identical.")

# Compare variable names
vars1 = set(ds1.variables.keys())
vars2 = set(ds2.variables.keys())

if vars1 != vars2:
    print("❌ Variable sets differ.")
    print("Only in file1:", vars1 - vars2)
    print("Only in file2:", vars2 - vars1)
else:
    print("✅ Variable sets are identical.")

# Compare variable contents
print("\n🔎 Comparing variable values...")
for var in sorted(vars1 & vars2):  # Only compare shared variables
    arr1 = ds1[var].values
    arr2 = ds2[var].values

    if not np.issubdtype(arr1.dtype, np.number):
        if not np.array_equal(arr1, arr2):
            print(f"⚠️ Non-numeric variable '{var}' differs.")
        continue

    if arr1.shape != arr2.shape:
        print(f"❌ Shape mismatch for variable '{var}': {arr1.shape} vs {arr2.shape}")
        continue

    diff = arr1 - arr2
    abs_diff = np.abs(diff)
    n_total = arr1.size
    n_diff = np.count_nonzero(abs_diff > 1e-8)  # Use tolerance for float comparison

    if n_diff == 0:
        print(f"✅ '{var}' is identical.")
    else:
        print(f"❌ '{var}' differs:")
        print(f"   → {n_diff} values differ out of {n_total} ({(n_diff/n_total)*100:.2f}%)")
        print(f"   → Min diff: {abs_diff.min():.3e}, Max diff: {abs_diff.max():.3e}, Mean diff: {abs_diff.mean():.3e}")