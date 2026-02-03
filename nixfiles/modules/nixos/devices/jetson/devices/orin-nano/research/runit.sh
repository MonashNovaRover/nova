#!bin/bash
llama-cli -m Hermes-3-Llama-3.2-3B-IQ4_XS.gguf -c 16384 -ngl 99 -fa 'on' --temp 1.0 --min-p 0.0 --top-p 1.0 --top-k 100 --mlock --jinja
