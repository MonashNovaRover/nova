/**
 * Author: Connor Macdougall
 * This component acts as a container for the three distinct components.
 * Use for simple layout configuration changes, and exporting.
 */

import RamanCCDInputs from "./RamanCCDInputs";
import RamanMechanicalInputs from "./RamanMechanicalInputs";
import RamanOutputComparison from "./RamanOutputComparison";

const RamanSpec: React.FC = () => {
    return (
        <div className="flex flex-col m-3 rounded-xl bg-zinc-800">
            <RamanMechanicalInputs />
            <RamanOutputComparison />
            <RamanCCDInputs />
        </div>
    )
}

export default RamanSpec;