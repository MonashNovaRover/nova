import {Button, Card, CardHeader} from "@nextui-org/react";
import {useGenericStore} from "../../../redux/actions/useGenericStore.ts";

/**
 * A card that displays a counter and increments and decrements it
 * @constructor
 */
export default function GenericStoreCard() {

    const {value: counter, setValue: setCounter} = useGenericStore<number>("counter");

    if (counter === undefined) {
      return <div/>
    }

    return (
        <Card className="flex flex-col grow">
            <CardHeader className="justify-center">
                useGenericStore
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