import {Button, Input} from "@nextui-org/react";
import {useResetGenericStore} from "../../../hooks/useResetGenericStore.ts";
import {useState} from "react";

/**
 * Store Settings component containing UI for resetting generic stores.
 * @constructor
 */
function StoreSettings() {
  const resetStore = useResetGenericStore()
  const [storeName, setStoreName] = useState("")
  const [result, setResult] = useState<boolean | undefined>(undefined)

  const onSubmit = () => {
    setResult(resetStore(storeName))
  }

  const onInputChange = (s: string) => {
    setStoreName(s)
    setResult(undefined)
  }

  const success = (
    <div className="text-green-500">
      Success!
    </div>
  )

  const warning = (
    <div className="text-danger">
      Caution: you will lose any associated data saved within local storage.
    </div>
  )

  return (
    <div className="flex flex-col gap-3 mt-4 mb-5">
      <Input
        fullWidth
        label="Reset Generic Store"
        labelPlacement="outside"
        value={storeName}
        onValueChange={onInputChange}
        errorMessage={result === false ? "Invalid generic store name" : ""}
        description={result === true ? success : warning}
        isInvalid={result === false}
        placeholder="Generic Store Name"
        type="text"
      />
      <div className="flex flex-row">
        <div className="grow"></div>
        <Button size="sm" color="success" variant="flat" onPress={onSubmit}>
          Reset
        </Button>
      </div>
    </div>
  )
}

export default StoreSettings;
