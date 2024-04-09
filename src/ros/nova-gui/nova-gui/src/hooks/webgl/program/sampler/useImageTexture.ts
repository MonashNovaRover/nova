import {useLayoutEffect, useRef} from "react";
import {loadImageFromURL} from "../../../../utils/webgl/loadTexture.ts";

/**
 * A hook, that creates a ref to an image at the given url, and returns the image
 * @param url The url of the image to display in the returned HTMLImageElement
 */
export default function useImageTexture(url: string) {
  const image = useRef<HTMLImageElement | undefined>(undefined);

  useLayoutEffect(() => {
    image.current = loadImageFromURL(url);
  }, [url]);

  return image.current;
}
