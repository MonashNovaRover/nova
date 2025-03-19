Python is a pain to package, so I'm just leaving it as a shell.nix

Use predict-cpu instead if cuda is not installed and you are getting an error.

```bash

cd ~/nova/src/other/ilmenite_ml/
nix-shell
python3 predict_ilmenite.py <path_to_data>

python3 predict_ilmenite_cpu.py <path_to_data>
```
