import {Button, Card, CardHeader} from "@nextui-org/react";
import {useLocalStorage} from "../../../hooks/useLocalStorage.ts";

export interface TwoCounters {
    counterOne: number,
    counterTwo: number,
}

/**
 * A card that displays a counter and increments and decrements it
 * @constructor
 */
export default function LSObjectCard() {

    const [twoCounters, setTwoCounters] = useLocalStorage("counterObj", {counterOne: 0, counterTwo: 0} as TwoCounters);

    return (
        <Card className="flex flex-col grow">
            <CardHeader className="justify-center">
                LS with Object
            </CardHeader>
            <div className="grid grid-cols-2 gap-4 mx-4 mt-2">
                <Button onClick={() => setTwoCounters({...twoCounters, counterOne: twoCounters.counterOne + 1})}>
                    +
                </Button>
                <Button onClick={() => setTwoCounters({...twoCounters, counterTwo: twoCounters.counterTwo + 1})}>
                    +
                </Button>
            </div>
            <div className="grid grid-cols-2 justify-center my-1">
                <CardHeader className="justify-center">
                    {twoCounters && `${twoCounters.counterOne}`}
                </CardHeader>
                <CardHeader className="justify-center">
                    {twoCounters && `${twoCounters.counterTwo}`}
                </CardHeader>
            </div>
            <div className="grid grid-cols-2 gap-4 mx-4 mb-4">
                <Button onClick={() => setTwoCounters({...twoCounters, counterOne: twoCounters.counterOne - 1})}>
                    -
                </Button>
                <Button onClick={() => setTwoCounters({...twoCounters, counterTwo: twoCounters.counterTwo - 1})}>
                    -
                </Button>
            </div>
        </Card>
    )
}