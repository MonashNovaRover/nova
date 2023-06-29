self: super:

{
  lib = super.lib.extend (libself: libsuper: import ../lib self libself);
}
