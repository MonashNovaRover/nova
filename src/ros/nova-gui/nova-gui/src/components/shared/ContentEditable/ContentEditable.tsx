import { useState, useEffect, FC, useCallback, HTMLProps, FormEvent, useMemo, useRef } from 'react';
import { renderToString } from "react-dom/server";
import useContextEditable from "./useContentEditable.ts";

export interface ContentEditableEvent {
  // Absolute offset of the cursor (read only)
  startOffset: number,
  endOffset: number,

  // Allows the cursor position to be set
  setCursorOffset: (absoluteStartOffset: number, absoluteEndOffset: number) => void
  cancelEvent: () => void,
}

export interface ContentEditableProps extends HTMLProps<HTMLDivElement> {
  onValueChange?: (value: string, event: ContentEditableEvent) => void;
  // Removes the span that wraps children, giving a worse typing experience.
  removeSpanWrapper?: boolean;
}

/**
 * A text input that allows for dynamic styling of input text. A very fancy text input.
 *
 * ! Some words of warning:
 * !   - Easy to cause infinite re-renders if your provided children don't match the modified text content given by
 * !     `props.onValueChange`.
 * !   - Does not work with undo and redo.
 * !   - Renders children manually. Given children should NOT be stateful.
 * @param props
 * @constructor
 */
const ContentEditable: FC<ContentEditableProps> = (props) => {
  const divRef = useRef<HTMLDivElement>(null);

  const context = useContextEditable();

  // Increasing number to trigger re-renders
  const [valueId, setValueId] = useState<number>(0);

  const getGrandchildren = useCallback((child: Node, defaultEmpty: Node[] = [],
                                        nodeToFind: Node | undefined = undefined) => {
    if (!child.hasChildNodes() || (nodeToFind !== undefined && child === nodeToFind))
      return child.textContent !== null && child.textContent.trim().length > 0 ? [child] : defaultEmpty;

    const children = [...child.childNodes.values()].filter(x =>
      x !== undefined && x !== null && x.textContent !== null && x.textContent.trim().length > 0)

    if (children.length === 0)
      return defaultEmpty;

    return children.flatMap((x: Node): Node[] => getGrandchildren(x))
  }, []);

  const textOf = useCallback((node: Node) => (
    node.textContent ?? ""
  ), []);

  const relativeToAbsoluteOffset = useCallback((grandchildren: Node[], idx: number, relativeOffset: number) => {
    if (idx >= grandchildren.length)
      return Infinity;

    if (idx == grandchildren.length - 1 && relativeOffset >= (grandchildren[grandchildren.length - 1].textContent?.length ?? 0))
      return Infinity;

    return grandchildren
      .slice(0, idx)
      .reduce((acc: number, val: Node): number => acc + textOf(val).length, 0)
      + relativeOffset;
  }, [textOf]);

  // Node ids are used to identify which element to add the cursor onto after the re-render
  const getNodeIdx = useCallback((node: Node, offset: number, grandchildren?: Node[]) => {
    const _grandchildren = grandchildren ?? getGrandchildren(divRef.current!, [divRef.current!], node)
    const idx = _grandchildren.indexOf(node as ChildNode)

    // Snap to end of string when typing at end of string
    if (idx >= _grandchildren.length - 1 && offset >= (_grandchildren[idx].textContent?.length ?? 0))
      return [Infinity, Infinity]

    if (idx === Infinity)
      return [Infinity, Infinity]

    return [idx, offset];
  }, [getGrandchildren]);

  // Saves the current cursor selection to context
  const saveSelection = useCallback(() => {
    const selection = window.getSelection();
    if (!selection || !selection.rangeCount) return;

    const range = selection.getRangeAt(0);

    const [startNodeIdx, startOffset] = getNodeIdx(range.startContainer, range.startOffset);
    const [endNodeIdx, endOffset] = getNodeIdx(range.endContainer, range.endOffset);

    // Save the start and end nodes along with their offsets
    const grandchildren = getGrandchildren(divRef.current!, [divRef.current!]);
    context.startOffset = relativeToAbsoluteOffset(grandchildren, startNodeIdx, startOffset);
    context.endOffset = relativeToAbsoluteOffset(grandchildren, endNodeIdx, endOffset);
  }, [context, getGrandchildren, getNodeIdx, relativeToAbsoluteOffset]);

  // Tries to put the cursor back into the saved position
  const applySelection = useCallback(() => {
    const selection = window.getSelection();

    if (!context || !selection || divRef.current === null)
      return;

    const { startOffset, endOffset } = context;

    const startNodeIdx = startOffset === Infinity ? Infinity : 0;
    const endNodeIdx = endOffset === Infinity ? Infinity : 0;

    const grandchildren = getGrandchildren(divRef.current!, [divRef.current!]);

    // Makes sure the given offset fits in the current node. Jumps to the next recursively node if needed.
    const normalizeRangeOffset = ([idx, offset]: [number, number]): [number, number] => {
      console.log(idx, offset)

      if (idx === undefined || idx < 0 || offset === undefined)
        return [0, 0];

      if (idx >= grandchildren.length) {
        return normalizeRangeOffset([grandchildren.length - 1, Infinity])
      }

      const node = grandchildren[idx];
      const textContent = node?.textContent ?? "";

      if (textContent.length > offset)
        return [idx, offset];

      // If this exceeds all content, just do the rightmost index
      if (idx >= grandchildren.length - 1)
        return [idx, textContent.length];

      // Move to the next grandchild if overflowing current text
      return normalizeRangeOffset([idx + 1, offset - textContent.length]);
    }

    const [normalStartIdx, normalStartOffset] = normalizeRangeOffset([startNodeIdx, startOffset]);
    const [normalEndIdx, normalEndOffset] = normalizeRangeOffset([endNodeIdx, endOffset]);

    console.log("normal start", normalStartIdx, normalStartOffset);
    console.log("normal end", normalEndIdx, normalEndOffset);
    console.log(grandchildren)

    const startNode = grandchildren[normalStartIdx];
    const endNode = grandchildren[normalEndIdx];

    // Create and apply new selection
    const range = document.createRange();

    if (!startNode || !endNode) {
      console.warn("Bad start node and end node for ContentEditable selection", startNode, endNode)
      return;
    }

    range.setStart(startNode, normalStartOffset);
    range.setEnd(endNode, normalEndOffset);

    selection.removeAllRanges();
    selection.addRange(range);
  }, [context, getGrandchildren]);

  // Called whenever the user modifies the div
  const handleInput = useCallback((event: FormEvent<HTMLDivElement>) => {
    const html = event.currentTarget.textContent ?? "no text content";

    saveSelection();

    const ceEvent = {
      startOffset: context.startOffset,
      endOffset: context.endOffset,
      setCursorOffset: (start: number, end: number) => {
        context.startOffset = start;
        context.endOffset = end;
      },
    } as ContentEditableEvent;

    props.onValueChange?.(html, ceEvent);
    setValueId(x => x + 1); // Trigger re-render
  }, [context, props, saveSelection]);

  const children = props.children;

  // Directly render the HTML to a string
  const html = useMemo<string>(() => (
    props.removeSpanWrapper
      ? renderToString(children)
      : "<span>" + renderToString(children) + "</span>"
  ), [children, props.removeSpanWrapper]);

  useEffect(() => {
    applySelection(); // Reapply selection after content change
  }, [applySelection, html, valueId]);

  return (
    <div
      contentEditable
      onInput={handleInput}
      {...props}
      children={undefined}
      dangerouslySetInnerHTML={{ __html: html }}
      ref={divRef}
      key={valueId}
    />
  );
};

export default ContentEditable;