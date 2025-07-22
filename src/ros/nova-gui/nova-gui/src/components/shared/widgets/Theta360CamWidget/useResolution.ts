import {useEffect, useState} from "react";


export default function useResolution(sizeTarget?: Element | null) {
  const [resolution, setResolution] = useState([1, 1]);

  // Keep track of resolution so we can pan properly
  useEffect(() => {
    if (!sizeTarget)
      return;

    const boxObserver = new ResizeObserver((entries) => {
      const entry = entries.find((entry) => entry.target === sizeTarget);

      if (!entry)
        return;

      const width = entry.devicePixelContentBoxSize[0].inlineSize;
      const height = entry.devicePixelContentBoxSize[0].blockSize;

      setResolution([width, height]);
    });
    boxObserver.observe(sizeTarget, { box: "device-pixel-content-box" });
  }, [resolution, sizeTarget]);

  return resolution;
}