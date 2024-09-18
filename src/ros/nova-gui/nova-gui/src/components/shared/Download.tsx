import { Button, ButtonProps } from '@nextui-org/react';
import { saveAs } from 'file-saver';


interface DownloadProps extends ButtonProps {
  content: string;
  filename: string;
  children: React.ReactNode;
}

const DownloadButton : React.FC<DownloadProps>= (props) => {
  const handleDownload = () => {
    const file = new Blob([props.content], { type: 'text/plain;charset=utf-8' });
    saveAs(file, props.filename);
  };

  return (
    <Button {...props} onPress={handleDownload}>
      {props.children}
    </Button>
  );
};

export default DownloadButton;