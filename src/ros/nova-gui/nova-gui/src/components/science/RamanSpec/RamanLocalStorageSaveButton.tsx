import React, {useState} from "react";
import {Button, Input} from "@nextui-org/react";


export interface RamanLocalStorageSaveButtonProps {
  onSave: (name: string) => void,
  onCSVSave?: () => void,
}

// A distinct component to contain the useState for graphName
const RamanLocalStorageSaveButton: React.FC<RamanLocalStorageSaveButtonProps> = (props) => {
  const [graphName, setGraphName] = useState<string>("");

  return (
    <div className="flex flex-row gap-3 my-3 mb-0">
      <Input size="sm" placeholder="Saved graph name" className="flex-grow" onValueChange={setGraphName} value={graphName}></Input>
      <Button
        color={graphName.length > 0 ? "primary" : "default"}
        isDisabled={graphName.length === 0}
        onPress={() => props.onSave?.(graphName)}
        size="sm"
      >
        Save
      </Button>
      {props.onCSVSave && (
        <Button color="success" onPress={props.onCSVSave} size="sm" className="px-5">
          Save to CSV
        </Button>
      )}
    </div>
  )
}

export default RamanLocalStorageSaveButton;



