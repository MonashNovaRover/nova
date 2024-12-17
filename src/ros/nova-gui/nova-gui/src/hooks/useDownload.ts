import {saveAs} from "file-saver";
import {DependencyList, useCallback} from "react";

export interface UseDownloadProps {
  // The [MIME Type](https://developer.mozilla.org/en-us/docs/Glossary/MIME_type) to use
  type: string;

}

/**
 * Similar to the DownlaodButton component, just with the logic in a reusable hook that returns a callback function.
 * If you vary the filename, that needs to go into the dependency array too.
 */
export default function useDownload(filename: string, callback: () => Blob | BlobPart | BlobPart[], deps: DependencyList,
                                    options?: Partial<UseDownloadProps>): () => void {
  // eslint-disable-next-line react-hooks/exhaustive-deps
  const wrappedCallback = useCallback(callback, deps);

  const filledOptions = {
    type: 'text/plain;charset=utf-8',
    ...options
  }

  // The callback function for downloading a file
  return useCallback(() => {
    // Get the content to save
    const blobOrPart = wrappedCallback();

    // Consolidate the various types accepted by this function to be a Blob
    const blob = blobOrPart instanceof Blob ? blobOrPart
      : new Blob(Array.isArray(blobOrPart) ? blobOrPart : [blobOrPart], { type: filledOptions.type });

    // Actually save the file
    saveAs(blob, filename);

  }, [wrappedCallback, filename, filledOptions.type]);
}

