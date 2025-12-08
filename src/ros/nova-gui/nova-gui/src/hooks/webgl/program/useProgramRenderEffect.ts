import React, {useEffect, useRef} from "react";
import GLProgramState from "./GLProgramState.ts";
import {RenderQueueItem} from "../render-queue/RenderQueue.ts";
import GLStateRenderInfo from "../gl/GLStateRenderInfo.ts";

export default function useProgramRenderEffect(program: GLProgramState,
                                               effect: (context: WebGL2RenderingContext, program: WebGLProgram,
                                                        info: GLStateRenderInfo) => void,
                                               deps: React.DependencyList = [])
{
  const renderQueueItem =
    useRef<RenderQueueItem<[WebGL2RenderingContext, WebGLProgram, GLStateRenderInfo]>>({
      setup: () => {},
      render: effect,
    });
  const itemID = useRef<number>(program.renderQueue.push(renderQueueItem.current!));

  if (renderQueueItem.current === undefined) {
    renderQueueItem.current = {
      setup: () => {},
      render: effect,
    }
  }

  useEffect(() => {
    if (itemID.current === undefined)
      itemID.current = program.renderQueue.push(renderQueueItem.current!);

    renderQueueItem.current!.render = effect;
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [...deps])
}
