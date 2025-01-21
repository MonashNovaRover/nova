import {useRef} from "react";

export interface ContentEditableContext {
  // Current absolute cursor offset (Read only). Currently, will not trigger re-renders, but this could be easily added.
  startOffset: number,
  endOffset: number,
}

export default function useContextEditable(): ContentEditableContext {
  const contextRef = useRef<ContentEditableContext | undefined>();

  if (contextRef.current === undefined) {
    contextRef.current = {
      startOffset: Infinity,
      endOffset: Infinity,
    };
  }

  return contextRef.current;
}















