import {useRef} from "react";
import RenderQueue from "./RenderQueue.ts";

export default function useRenderQueue() {
  const queue = useRef<RenderQueue>();
  if (queue.current === undefined)
    queue.current = new RenderQueue();

  return queue.current;
}