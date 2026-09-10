# M2 — Synthetic translated-product link probe

Status: CI + real-hardware validation required before this checkpoint is complete.

Issue: #21.

## Goal

Prove the translated-product weak/strong linker seam independently of Mario Kart Wii generated output.

This probe contains **no Nintendo data**. It exists so the exact linker behavior used by a future local generated product can be validated before any game-derived sources are involved.

## Builds

Default public runtime probe:

```sh
make -j2
```

Output:

```text
WiiCompiled-Switch.nro
```

The default build keeps the weak product provider and should stop at:

```text
WAITING_FOR_TRANSLATED_PRODUCT
```

Synthetic strong-product probe:

```sh
make -j2 MKW_SYNTHETIC_PRODUCT=1
```

Output:

```text
WiiCompiled-Switch-synthetic-product.nro
```

The synthetic build compiles `synthetic-product/synthetic_translated_product.cpp`, which provides a strong definition of:

```text
mkw_switch_get_translated_product_api
```

The product metadata is intentionally generic:

```text
product id    : synthetic-ci-product
build         : Nintendo-data-free strong-link probe
ABI           : 1
```

No data-section initializer and no translated guest function are included or invoked.

## CI contract

CI builds both targets and inspects the ELF symbol table:

- default ELF must expose the product query as weak (`W`);
- synthetic ELF must expose the product query as strong text (`T`).

Both NROs remain Nintendo-data-free and may be uploaded as public CI artifacts.

## Expected Switch report

Launch the synthetic NRO through hbmenu application/title-override mode with full memory.

`runtime-bootstrap.txt` should keep the existing M2 core READY and report:

```text
translated product     : LINKED
translated product ABI : expected=1 reported=1
translated product id  : synthetic-ci-product
translated build       : Nintendo-data-free strong-link probe
stop point             : TRANSLATED_PRODUCT_LINKED
```

Audio and graphics should remain `STUBBED`.

This is still a pre-execution checkpoint. `TRANSLATED_PRODUCT_LINKED` means only that the strong product seam is working; it does **not** mean Mario Kart Wii code ran.

## Next step after hardware PASS

Once the synthetic strong-link probe passes on real Switch hardware, the same seam can be used by a local-only generated product adapter. The next staged boundary is then the translator-generated `InitializeDataSections()` call, followed later by the first translated constructor/entry-point handoff.
