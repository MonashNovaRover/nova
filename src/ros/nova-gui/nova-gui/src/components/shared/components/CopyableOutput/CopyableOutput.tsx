import { Button } from "@heroui/react";
import React from "react";
import { Copy } from "react-feather";

interface CopyableOutputProps {
  children?: React.ReactNode;
  className?: string;
  classNames?: {
    base?: string;
    pre?: string;
  };
}

const CopyableOutput: React.FC<CopyableOutputProps> = ({ children, className, classNames }) => {
  const copyValue = () => navigator.clipboard.writeText(String(children ?? ""));

  return (
    <div className={`relative flex items-center justify-between overflow-hidden ${className ?? ""} ${classNames?.base ?? ""}`}>
      <output className={`flex-1 text-center ${classNames?.pre ?? ""}`}>{children}</output>
      <Button aria-label="Copy to clipboard" isIconOnly size="sm" variant="ghost" onPress={copyValue}>
        <Copy size={16} />
      </Button>
    </div>
  );
};

export default CopyableOutput;