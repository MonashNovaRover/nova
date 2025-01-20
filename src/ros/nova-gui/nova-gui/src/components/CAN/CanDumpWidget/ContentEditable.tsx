import { useState, useEffect, FC, useCallback, HTMLProps, FormEvent, useMemo, useRef } from 'react';
import { renderToString } from "react-dom/server";

export interface ContentEditableProps extends HTMLProps<HTMLDivElement> {
  onValueChange?: (value: string) => void;
  // Removes the span that wraps children, giving a worse typing experience.
  removeSpanWrapper?: boolean;
}

const ContentEditable: FC<ContentEditableProps> = (props) => {
  const divRef = useRef<HTMLDivElement>(null);
  const selectionRangesRef = useRef<{ startNodeIdx: number, endNodeIdx: number, startOffset: number, endOffset: number } | null>(null);

  // Increasing number to trigger re-renders
  const [valueId, setValueId] = useState<number>(0);

  // Node ids are used to identify which element to add the cursor onto after the re-render
  const getNodeIdx = (node: Node, offset: number) => {
    const getGrandchildren = (child: Node, defaultEmpty: Node[] = []) => {
      if (!child.hasChildNodes() || child === node)
        return child.textContent !== null && child.textContent.trim().length > 0 ? [child] : defaultEmpty;

      const children = [...child.childNodes.values()].filter(x =>
        x !== undefined && x !== null && x.textContent !== null && x.textContent.trim().length > 0)

      if (children.length === 0)
        return defaultEmpty;

      return children.flatMap((x: Node): Node[] => getGrandchildren(x))
    };

    const grandchildren = getGrandchildren(divRef.current!, [divRef.current!])
    const idx = grandchildren.indexOf(node as ChildNode)

    // Snap to end of string when typing at end of string
    if (idx >= grandchildren.length - 1 && offset >= (grandchildren[idx].textContent?.length ?? 0))
      return [Infinity, Infinity]

    if (idx === Infinity)
      return [Infinity, Infinity]

    return [idx, offset];
  }

  // Saves the current cursor selection to selectionRangesRef, and incre
  const saveSelection = () => {
    const selection = window.getSelection();
    if (!selection || !selection.rangeCount) return;

    const range = selection.getRangeAt(0);

    const [startNodeIdx, startOffset] = getNodeIdx(range.startContainer, range.startOffset);
    const [endNodeIdx, endOffset] = getNodeIdx(range.endContainer, range.endOffset);

    // Save the start and end nodes along with their offsets
    selectionRangesRef.current = {
      startNodeIdx: startNodeIdx,
      startOffset: startOffset,
      endNodeIdx: endNodeIdx,
      endOffset: endOffset,
    };
  };

  // Tries to put the cursor back into the saved position
  const applySelection = () => {
    const selection = window.getSelection();

    if (!selectionRangesRef.current || !selection || divRef.current === null)
      return;

    const { startNodeIdx, endNodeIdx, startOffset, endOffset } = selectionRangesRef.current;

    const getGrandchildren = (child: Node, defaultEmpty: Node[] = []) => {
      if (!child.hasChildNodes())
        return child.textContent !== null && child.textContent.trim().length > 0 ? [child] : defaultEmpty;

      const children = [...child.childNodes.values()].filter(x =>
        x !== undefined && x !== null && x.textContent !== null && x.textContent.trim().length > 0)

      if (children.length === 0)
        return defaultEmpty;

      return children.flatMap((x: Node): Node[] => getGrandchildren(x))
    };

    const grandchildren = getGrandchildren(divRef.current!, [divRef.current!]);
    console.log(grandchildren)

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
  };

  // Called whenever the user modifies the div
  const handleInput = useCallback((event: FormEvent<HTMLDivElement>) => {
    const html = event.currentTarget.textContent ?? "no text content";
    props.onValueChange?.(html);

    saveSelection();
    setValueId(x => x + 1); // Trigger re-render
  }, [props]);

  const children = props.children;

  // Directly render the HTML to a string
  const html = useMemo<string>(() => (
    props.removeSpanWrapper
      ? renderToString(children)
      : "<span>" + renderToString(children) + "</span>"
  ), [children, props.removeSpanWrapper]);

  useEffect(() => {
    applySelection(); // Reapply selection after content change
  }, [html, valueId]);

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