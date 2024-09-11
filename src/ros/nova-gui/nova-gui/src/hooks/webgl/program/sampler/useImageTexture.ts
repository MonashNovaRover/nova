import {useLayoutEffect, useRef} from "react";

/**
 * A hook, that creates a ref to an image at the given url, and returns the image
 * @param url The url of the image to display in the returned HTMLImageElement
 */
export default function useImageTexture(url: string) {
  const image = useRef<HTMLImageElement | undefined>(undefined);

  if (image.current === undefined) {
    image.current = new Image();
  }

  useLayoutEffect(() => {
    if (image.current === undefined)
      return;

    image.current.src = url;
  }, [url]);

  return image.current;
}
