call plug#begin()
Plug 'python-mode/python-mode', { 'for': 'python', 'branch': 'develop' }
call plug#end()

set backspace=indent,eol,start

set number
set showmatch " Shows matching brackets
set ruler " Always shows location in file (line#)
set smarttab " Autotabs for certain code
set shiftwidth=4
set tabstop=4

let g:pymode_rope = 1
let g:pymode_rope_completion = 1
"let g:pymode_rope_completion_bind = '<C-Space>'
let g:pymode_rope_complete_on_dot = 0

syntax on
filetype plugin indent on
filetype indent on
