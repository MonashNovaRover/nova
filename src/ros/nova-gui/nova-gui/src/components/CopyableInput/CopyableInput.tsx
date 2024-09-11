import { Button, Input, InputProps, Tooltip } from "@nextui-org/react";
import React, { useCallback } from "react";
import { Copy } from "react-feather";

export interface CopyableInputProps extends InputProps {
  // The value to copy to the clipboard when the copy button is pressed. Otherwise uses value by default
  copyValue?: string;
}

/**
 * A component based on Next UI's Input class that displays a value in a box, with a copy button on it's right.
 * @constructor
 */
const CopyableInput: React.FC<CopyableInputProps> = ({
  children: children,
  endContent,
  value,
  copyValue,
  ...inputProps
}) => {
  const copyValueToClipboard = useCallback(() => {
    navigator.clipboard.writeText(copyValue ?? value?.toString() ?? "");
  }, [value, copyValue]);

  const copyButton = (
    <Tooltip
      content={"Copy to Clipboard"}
      placement="bottom"
      showArrow
      className="dark text-foreground"
    >
      <Button
        isIconOnly
        size="sm"
        variant="light"
        onPress={copyValueToClipboard}
      >
        <Copy size="16" />
      </Button>
    </Tooltip>
  );

  const newEndContent = (
    <div className="flex gap-1 -mr-2 items-center">
      {endContent}
      {copyButton}
    </div>
  );

  return (
    <Input value={value} {...inputProps} endContent={newEndContent}>
      {children}
    </Input>
  );
};

export default CopyableInput;
