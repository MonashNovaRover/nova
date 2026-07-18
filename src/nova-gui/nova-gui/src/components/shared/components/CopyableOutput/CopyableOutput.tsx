import {Snippet, SnippetProps} from "@nextui-org/react";
import React from "react";

/**
 * A component based on Next UI's Snippet class that displays a value in a box, with a copy button on it's right.
 * @constructor
 */
const CopyableOutput: React.FC<SnippetProps> = ({
  hideSymbol: hideSymbol,
  className: className,
  classNames: classNames,
  children: children,
  ...props
}) => {

  return (
    <Snippet
      {...props}
      hideSymbol={hideSymbol ?? true}
      className={"overflow-hidden " + (className ?? "")}
      classNames={{
        ...classNames,
        pre: "absolute top-0 left-0 right-0 bottom-0 flex flex-col justify-around content-center "
          + (classNames?.pre ?? ""),
        base: "justify-end relative overflow-hidden" + (classNames?.base ?? ""),
      }}
    >
      <div className=" text-center">{children}</div>
    </Snippet>
  );
}

export default CopyableOutput;