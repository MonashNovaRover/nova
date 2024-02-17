import {useLayoutEffect, useRef} from "react";
import {loadImageFromURL} from "../webgl-utils/loadTexture.ts";


export default function useImageTexture(url: string) {

  const image = useRef<HTMLImageElement | undefined>(undefined);

  useLayoutEffect(() => {
    image.current = loadImageFromURL(url);
  }, [url]);

  return image.current;
}