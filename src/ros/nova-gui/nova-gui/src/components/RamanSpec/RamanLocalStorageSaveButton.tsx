import React, {useState} from "react";
import {Button, Input} from "@nextui-org/react";


export interface RamanLocalStorageSaveButtonProps {
  onSave: (name: string) => void
}

// A distinct component to contain the useState for graphName
const RamanLocalStorageSaveButton: React.FC<RamanLocalStorageSaveButtonProps> = (props) => {
  const [graphName, setGraphName] = useState<string>("CCD Output");

  return (
    <div className="flex flex-row gap-3 my-3 mb-0">
      <Input size="sm" placeholder="Saved graph name" onValueChange={setGraphName} value={graphName}></Input>
      <Button
        color={graphName.length > 0 ? "success" : "default"}
        isDisabled={graphName.length === 0}
        onPress={() => props.onSave?.(graphName)}
        size="sm"
      >
        Save
      </Button>
    </div>
  )
}

export default RamanLocalStorageSaveButton;



