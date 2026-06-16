---
name: novapanel-host-check
description: Validate NovaPanel firmware C++ on the host without ESP-IDF by running tools/scripts/host_check.sh and interpreting shim compile results. Use after changing firmware/components or firmware/main, before committing firmware changes, or when checking type/include/interface errors quickly.
---

# NovaPanel Host Check

Use this as the fast C++ pre-flight.

## Command

From repo root:

```bash
bash tools/scripts/host_check.sh --app
```

Omit `--app` only when you intentionally want to skip `firmware/main/app_main.cpp`.

## Interpretation

- `OK` means the source compiled against host shims.
- `FAIL` prints the compiler error below the file.
- Fix source/header issues; do not edit generated shim headers.

## Common failures

- Missing include (`<string>`, `<vector>`, `<cstdint>`, `<cstddef>`).
- Header/source signature drift.
- Component include path assumptions.
- Service/provider interface mismatch.

## Important

This does not replace `idf.py build`. After host check passes, use ESP-IDF build as the final firmware validation when practical.
