# Credits Gambling: behavior and implementation

Describes the exported implementation **0.1.0**. The linked source and metadata define this version; gameplay qualification is a separate requirement.

## Behavior

Ctrl+G wagers virtual in-game credits on a host-only chance event. Defaults are a $5 stake, 45% win probability, twice-stake gross payout and four-second cooldown. A BCrypt random byte is rejection-sampled before committing; native wallet/debit methods own the economy changes and before/after balances verify the result. No external service or real-money operation is involved.

## State and lifecycle

The INI stores bounded stake/chance/cooldown settings. Credits persist through normal game saves; this mod introduces no separate gambling ledger or automatic save. A transaction mismatch disables further wagers for the process. Input requires a ready local-host world, focus and no blocking UI/typing.

## Integration and configuration

No additional feature-mod runtime dependency is declared. Included shared headers are source reuse, not extra DLL dependencies.

The [module metadata](../source/mods/gambling/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

Packaged defaults in [settings.ini](../source/mods/gambling/settings.ini); supported bounds and interpretation remain in the linked parser.

| Section | Setting | Default |
|---|---|---|
| `[gambling]` | `bet` | `5` |
| `[gambling]` | `win_percent` | `45` |
| `[gambling]` | `cooldown_seconds` | `4` |

## Implementation sources

- [mod.c](../source/mods/gambling/mod.c)
- [support.h](../source/mods/gambling/support.h)

## Verification boundary

No multiplayer betting, reward inventory, house economy or crash-atomic multi-step transaction. Wallet settlement, configured probabilities and save round trips require game-level checks.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
