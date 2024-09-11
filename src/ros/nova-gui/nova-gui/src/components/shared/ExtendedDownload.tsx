import { Button, ButtonProps } from '@nextui-org/react';
import { saveAs } from 'file-saver';
import React from "react";

export type ExtendedContent = ArrayBuffer | DataView | Blob | string;

interface ExtendedDownloadProps extends ButtonProps {
  fileContent: BlobPart[] | (() => BlobPart[]);
  filename: string;
  // The file type descriptor to use when saving. E.g. "text/plain;charset=utf-8"
  fileType?: string;
  children?: React.ReactNode;
}

const ExtendedDownloadButton : React.FC<ExtendedDownloadProps>= (props) => {
  const filetype = props.type ?? "text/plain;charset=utf-8";

  const handleDownload = () => {
    const content = typeof props.fileContent === 'function' ? props.fileContent() : props.fileContent

    const file = new Blob(content, { type: filetype });
    saveAs(file, props.filename);
  };

  return (
    <Button {...props} onPress={handleDownload}>
      {props.children}
    </Button>
  );
};

export default ExtendedDownloadButton;