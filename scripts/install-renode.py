#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import platform
import shutil
import stat
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path

REPO_API = "https://api.github.com/repos/renode/renode/releases/latest"


def download_file(url: str, destination: Path) -> None:
    request = urllib.request.Request(url, headers={"User-Agent": "robot-arm-setup"})
    with urllib.request.urlopen(request) as response, destination.open("wb") as output:
        shutil.copyfileobj(response, output)


def latest_release_assets() -> list[dict[str, str]]:
    request = urllib.request.Request(REPO_API, headers={"User-Agent": "robot-arm-setup"})
    with urllib.request.urlopen(request) as response:
        payload = json.load(response)
    return payload["assets"]


def find_asset(assets: list[dict[str, str]], patterns: list[str]) -> dict[str, str]:
    for pattern in patterns:
        for asset in assets:
            if pattern in asset["name"]:
                return asset
    raise RuntimeError(f"Could not find a Renode release asset matching any of: {patterns}")


def extract_archive(archive: Path, install_dir: Path) -> None:
    if archive.suffix == ".zip":
        with zipfile.ZipFile(archive) as zip_file:
            zip_file.extractall(install_dir)
        return

    if archive.name.endswith(".tar.gz"):
        with tarfile.open(archive, "r:gz") as tar_file:
            tar_file.extractall(install_dir)
        return

    raise RuntimeError(f"Unsupported archive format: {archive}")


def find_executable(install_dir: Path) -> Path:
    candidates = [*install_dir.rglob("renode.exe"), *install_dir.rglob("renode")]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise RuntimeError(f"Renode executable was not found after extraction under {install_dir}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Install Renode into the repo-local .tooling directory.")
    parser.add_argument("--root", required=True, help="Repository root directory")
    args = parser.parse_args()

    root_dir = Path(args.root).resolve()
    tooling_dir = root_dir / ".tooling"
    install_dir = tooling_dir / "renode"
    install_dir.mkdir(parents=True, exist_ok=True)

    system = platform.system().lower()
    if system == "linux":
        asset_patterns = ["linux-portable-dotnet.tar.gz", "linux-portable.tar.gz"]
    elif system == "windows":
        asset_patterns = ["windows-portable-dotnet.zip"]
    else:
        raise RuntimeError(
            f"Unsupported platform for scripted Renode install: {platform.system()}. "
            "On macOS, install Renode with Homebrew and keep the local launch scripts pointed at that binary."
        )

    assets = latest_release_assets()
    asset = find_asset(assets, asset_patterns)

    with tempfile.TemporaryDirectory() as temp_dir_name:
        temp_dir = Path(temp_dir_name)
        archive_path = temp_dir / asset["name"]
        print(f"Downloading Renode asset: {asset['name']}")
        download_file(asset["browser_download_url"], archive_path)

        print(f"Extracting to: {install_dir}")
        extract_archive(archive_path, install_dir)

    executable = find_executable(install_dir)
    if system == "linux":
        executable.chmod(executable.stat().st_mode | stat.S_IEXEC)

    bin_dir = tooling_dir / "bin"
    bin_dir.mkdir(parents=True, exist_ok=True)

    if system == "windows":
        wrapper = bin_dir / "renode.cmd"
        wrapper.write_text(
            "@echo off\r\n"
            f'"{executable}" %*\r\n',
            encoding="utf-8",
        )
    else:
        wrapper = bin_dir / "renode"
        wrapper.write_text(
            "#!/usr/bin/env bash\n"
            "set -euo pipefail\n"
            f'exec "{executable}" "$@"\n',
            encoding="utf-8",
        )
        wrapper.chmod(wrapper.stat().st_mode | stat.S_IEXEC)

    print(f"Renode installed: {executable}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())