Python is a pain to package, so I'm just leaving it as a shell.nix

```bash

cd ~/nova/src/other/ilmenite_ml/
nix-shell
python3 predict_ilmenite.py <path_to_data>
```