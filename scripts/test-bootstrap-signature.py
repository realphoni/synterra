"""Regression: import an armored key file, verify a signature, reject tampering.

Uses throwaway local signing keys; does not fetch or trust new Arch keys.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile

gpg = shutil.which('gpg')
if not gpg:
    raise SystemExit('gpg is required for this regression test')

def native_path(path):
    cygpath = Path(gpg).with_name('cygpath.exe')
    if cygpath.is_file():
        return subprocess.check_output([str(cygpath), '-u', str(path)], text=True).strip()
    return str(path)

with tempfile.TemporaryDirectory(prefix='synterra-gpg-test-') as folder:
    root = Path(folder)
    signer, verifier = root / 'signer', root / 'verifier'
    signer.mkdir(mode=0o700)
    verifier.mkdir(mode=0o700)

    def run(home, *args, check=True):
        result = subprocess.run(
            [gpg, '--no-options', '--homedir', native_path(home), '--batch', *args],
            capture_output=True,
        )
        if check and result.returncode:
            raise RuntimeError(result.stderr.decode(errors='replace'))
        return result

    try:
        run(signer, '--pinentry-mode', 'loopback', '--passphrase', '',
            '--quick-generate-key', 'Synterra regression <test@example.invalid>',
            'ed25519', 'sign', '0')
        key = root / 'archlinux.gpg'
        key.write_bytes(run(signer, '--armor', '--export').stdout)
        assert key.read_bytes().startswith(b'-----BEGIN PGP PUBLIC KEY BLOCK-----')
        archive = root / 'bootstrap.fixture'
        archive.write_bytes(b'Synterra bootstrap regression fixture\n')
        signature = root / 'bootstrap.fixture.sig'
        run(signer, '--pinentry-mode', 'loopback', '--passphrase', '',
            '--output', native_path(signature), '--detach-sign', native_path(archive))
        run(verifier, '--import', native_path(key))
        run(verifier, '--verify', native_path(signature), native_path(archive))
        archive.write_bytes(b'Tampered bootstrap fixture\n')
        assert run(verifier, '--verify', native_path(signature), native_path(archive),
                   check=False).returncode != 0
        print('PASS: armored key import, valid signature, and tamper rejection.')
    finally:
        gpgconf = shutil.which('gpgconf')
        if gpgconf:
            for home in (signer, verifier):
                subprocess.run([gpgconf, '--homedir', native_path(home), '--kill', 'all'],
                               capture_output=True)
