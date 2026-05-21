import React, {useState} from "react";
import {Button, Input} from "@nextui-org/react";


export interface RamanLocalStorageSaveButtonProps {
  onSave: (name: string) => void,
  onCSVSave?: () => void,
  suggestedName?: string,  // Auto-populated name based on carousel position
}

// A distinct component to contain the useState for graphName
// Uses key prop from parent to reset when suggestedName changes
const RamanLocalStorageSaveButton: React.FC<RamanLocalStorageSaveButtonProps> = (props) => {
  const [graphName, setGraphName] = useState<string>(props.suggestedName ?? "");

  // Track previous suggestedName to sync state when prop changes (React recommended pattern)
  const [prevSuggestedName, setPrevSuggestedName] = useState(props.suggestedName);
  if (props.suggestedName !== prevSuggestedName) {
    setPrevSuggestedName(props.suggestedName);
    if (props.suggestedName !== undefined) {
      setGraphName(props.suggestedName);
    }
  }

  return (
    <div className="flex flex-row gap-3 my-3 mb-0 flex-grow">
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
        <Button color="default" onPress={props.onCSVSave} size="sm" className="px-5">
          Save to CSV
        </Button>
      )}
    </div>
  )
}

export default RamanLocalStorageSaveButton;



