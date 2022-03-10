import { useState } from 'react';

const UNITS = ['B', 'KB', 'MB', 'GB', 'TB'];

interface PaneTxProps {
  bytesTx: number;
  pktTx: number;
}

function convertToUnit(bytes: number, unitIndex: number): number {
  switch (unitIndex) {
    case 0:
      return bytes;
    case 1:
      return bytes / 1000;
    case 2:
      return bytes / 1000000;
    case 3:
      return bytes / 1000000000;
    case 4:
      return bytes / 1000000000000;
    default:
      return bytes;
  }
}

function PaneTx(props: PaneTxProps) {
  const [unitIndex, setUnitIndex] = useState(1);

  return (
    <div className="block-tx-rx">
      <div className="rounded-full py-4 px-5 bg-blue-200 font-bold text-blue-700 xl:text-xl">
        Tx
      </div>

      <div className="w-full flex justify-evenly">
        <div className="flex flex-col text-right justify-center">
          <div className="text-2xl font-bold xl:text-4xl">{props.pktTx}</div>
          <div className="text-sm mt-[-0.5rem] xl:text-md">PACKETS</div>
        </div>

        <div className="flex items-center space-x-2">
          <div className="text-4xl font-bold text-right min-w-[4rem] xl:text-6xl">
            {unitIndex == 0
              ? convertToUnit(props.bytesTx, unitIndex)
              : convertToUnit(props.bytesTx, unitIndex).toFixed(2)}
          </div>
          <button
            className="button-unit"
            onClick={() => setUnitIndex((unitIndex + 1) % UNITS.length)}
          >
            {UNITS[unitIndex]}
          </button>
        </div>
      </div>
    </div>
  );
}

export default PaneTx;
