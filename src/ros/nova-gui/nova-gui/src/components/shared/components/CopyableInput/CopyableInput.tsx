import { Button, Input, InputProps } from "@heroui/react";
import React, { useCallback } from "react";
import { Copy } from "react-feather";

export interface CopyableInputProps extends Omit<InputProps, "children"> {
  // The value to copy to the clipboard when the copy button is pressed. Otherwise uses value by default
  copyValue?: string;
  label?: React.ReactNode;
  endContent?: React.ReactNode;
  children?: React.ReactNode;
}

/**
 * A component based on Next UI's Input class that displays a value in a box, with a copy button on it's right.
 * @constructor
 */
const CopyableInput: React.FC<CopyableInputProps> = ({
  endContent,
  label,
  value,
  copyValue,
  ...inputProps
}) => {
  const copyValueToClipboard = useCallback(() => {
    navigator.clipboard.writeText(copyValue ?? value?.toString() ?? "");
  }, [value, copyValue]);

  const copyButton = (
    <Button
      aria-label="Copy to clipboard"
      isIconOnly
      size="sm"
      variant="ghost"
      onPress={copyValueToClipboard}
    >
      <Copy size="16" />
    </Button>
  );

  const newEndContent = (
    <div className="flex gap-1 -mr-2 items-center">
      {endContent}
      {copyButton}
    </div>
  );

  return <label className="flex flex-col gap-1">{label}<span className="flex items-center gap-1"><Input value={value} {...inputProps} />{newEndContent}</span></label>;
};

export default CopyableInput;
