/**
 * Author: Connor Macdougall
 * This component outputs responses from the CCD, and also has graphs to compare output data against.
 * It accepts responses from the 'raman_spectra' ROS service.
 */

import {Card, CardHeader, ScrollShadow} from "@nextui-org/react";

const RamanOutputComparison: React.FC = () => {
    return (
        <Card className="w-fit p-2 m-1 w-auto">
            <CardHeader className="shrink-0 w-48 p-1">Comparison and Analysis</CardHeader>
            <div className="flex flex-row">
                <ScrollShadow hideScrollBar className="w-1/2 h-1/2">
                </ScrollShadow>
            </div>
        </Card>
    )
}

export default RamanOutputComparison;