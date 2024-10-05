# Sops

We use [sops-nix](https://github.com/Mic92/sops-nix), based on [getsops](https://github.com/getsops/sops), to encrypt/decrypt our secrets.

## Setup
_Derived from [usage steps](https://github.com/Mic92/sops-nix?tab=readme-ov-file#usage-example)_

```
mkdir -p /var/lib/sops-nix
age-keygen -o /var/lib/sops-nix/keys.txt
```

Add a new entry for your public key under `keys` in `~/nixfiles/modules/nixos/secrets/.sops.yaml`, like this:

```
keys:
    - &bob age12zlz6lvcdk6eqaewfylg35w0syh58sm7gh53q5vvn7hd7c6nngyseftjxl
    - &<your_name> <your_public_key>
```

Add `*<your_name>` under the `age` key group, like this:

```
creation_rules:
  - path_regex: .yaml
    key_groups:
    - age:
      - *bob
      - *<your_name>
```

Make a PR. 
An existing member will update the keys for all the relevant secrets and merge your PR into `master`.
Pull from master. You will now be able to decrypt the secrets on your local machine.