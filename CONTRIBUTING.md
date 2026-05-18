# Contributing

Thank you for your interest in this project. It is primarily a portfolio / learning
repository; contributions are welcome but may be reviewed slowly.

## Where to start

- **Firmware (STM32):** Read [`firmware/CONTRIBUTING.md`](firmware/CONTRIBUTING.md)
  for the tiered learning scaffold and native test workflow.
- **Tests:** See [`TESTING.md`](TESTING.md) for commands that mirror CI.
- **Architecture:** Start at [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Development setup

```bash
./setup.sh          # Linux/macOS — venv, ROS CLI tooling, Renode
./firmware/scripts/test.sh
./simulation/scripts/test.sh
```

ROS / MoveIt work uses `./dev.sh` and Docker; see [`software/README.md`](software/README.md).

## Commit conventions (going forward)

- Subject line ≤ 72 characters; use [Conventional Commits](https://www.conventionalcommits.org/)
  prefixes where practical (`feat:`, `fix:`, `docs:`, `chore:`).
- Prefer `git pull --rebase` before pushing to `main` to avoid merge commits.
- Do not commit build artifacts (`build/`, `install/`, `log/`, `.pio/`, `compile_commands.json`).

## Documentation

If you change behavior, update the relevant doc under `docs/` or the subsystem README.
Simulink / codegen paths are **planned but not in the tree** — see banners in `simulink/`.

## License

By contributing, you agree that your contributions are licensed under the MIT License
in [`LICENSE`](LICENSE).
