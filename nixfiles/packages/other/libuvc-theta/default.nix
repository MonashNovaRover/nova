{ libuvc
, fetchpatch
}:

libuvc.overrideAttrs ({ patches ? [ ], ... }: {
  patches = patches ++ [
    (fetchpatch {
      url = "https://github.com/ricohapi/libuvc-theta/commit/092cf64c2c942a2fa985f56cb5f69e7407141a2f.patch";
      hash = "sha256-S9StMJyW/vlHb7IFOavawwneOpWjJ0eq1o/lDXkZb2Y=";
    })
  ];
})
