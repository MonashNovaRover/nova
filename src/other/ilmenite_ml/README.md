Python is a pain to package, so I'm just leaving it as a shell.nix

Start the ilmenite prediction shell using the command below, and use the 'predict' command to generate predictions. If cuda is not installed, or you are getting an error, use 'predict-cpu' to forcibly run it on CPU instead.

```bash
predict-shell
predict <path_to_data>
predict-cpu <path_to_data>
```
