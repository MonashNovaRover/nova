import {Button, Card, CardHeader} from "@nextui-org/react";
import {usePersistentState} from "../../../hooks/usePersistantState.ts";

/**
 * A card that displays a counter and increments and decrements it
 * @constructor
 */
export default function PersistentStoreCard() {

    const [counter, setCounter] = usePersistentState<number>("counter", 0);

    if (counter === undefined) {
      return <div/>
    }

    return (
        <Card className="flex flex-col grow">
            <CardHeader className="justify-center">
                usePersistentState
            </CardHeader>
            <div className="grid gap-4 mx-4 mt-2">
                <Button onClick={() => setCounter(counter + 1)}>
                    +
                </Button>
            </div>
            <div className="grid justify-center my-1">
                <CardHeader className="justify-center">
                    {counter}
                </CardHeader>
            </div>
            <div className="grid gap-4 mx-4 mb-4">
                <Button onClick={() => setCounter(counter - 1)}>
                    -
                </Button>
            </div>
        </Card>
    )
}