" Set up plugins. Requires vim-plug
call plug#begin()

" Python-mode, a Python IDE for Vim
Plug 'python-mode/python-mode', { 'for': 'python', 'branch': 'develop' }

call plug#end()

" -------------------------------------------

" Indent rules
set expandtab " Always replace tabs with spaces
set softtabstop=4 " Set number of indent spaces when 'tab' key is pressed
set shiftwidth=4 " Set number of indent spaces for a level of indentation

" User interface rules
set backspace=indent,eol,start " Allow backspacing over indents, line endings and insert-start-point
set mouse=a " Enable mouse for scrolling and reizing
set number " Show line numbers
set ruler " Always show cursor position in bottom right of screen
set showmatch " Shows matching brackets
set visualbell " Flash screen instead of making error noise

" File rules
filetype plugin indent on " Set filetype detection and other useful things
syntax on " Set syntax highlighting for the detected filetype

" Options for Python-mode
" Autocomplete options
let g:pymode_rope = 1
let g:pymode_rope_completion = 1
"let g:pymode_rope_completion_bind = '<C-Space>'
let g:pymode_rope_complete_on_dot = 0

