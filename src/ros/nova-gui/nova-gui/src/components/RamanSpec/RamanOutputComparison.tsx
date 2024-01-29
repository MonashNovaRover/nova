/**
 * Author: Connor Macdougall
 * This component outputs responses from the CCD, and also has graphs to compare output data against.
 * It accepts responses from the 'raman_spectra' ROS service.
 */

import {Card} from "@nextui-org/react";

const RamanOutputComparison: React.FC = () => {
    return (
        <Card className="w-fit p-2 m-1">Output Comparison
        </Card>
    )
}

export default RamanOutputComparison;