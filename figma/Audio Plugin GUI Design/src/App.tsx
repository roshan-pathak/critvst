import { useState } from 'react';
import { RotaryKnob } from './components/RotaryKnob';
import { MatrixDisplay } from './components/MatrixDisplay';

export default function App() {
  const [pressure, setPressure] = useState(0);
  const [fault, setFault] = useState(0);
  const [split, setSplit] = useState(0);
  const [momentum, setMomentum] = useState(0);
  const [resolve, setResolve] = useState(0);
  const [mix, setMix] = useState(100);
  const [preset, setPreset] = useState('');

  const handleSave = () => {
    const presetData = {
      pressure,
      fault,
      split,
      momentum,
      resolve,
      mix
    };
    console.log('Saving preset:', presetData);
    // In a real plugin, this would save to the DAW or file system
  };

  const handleLoadPreset = (presetName: string) => {
    setPreset(presetName);
    // In a real plugin, this would load preset values
  };

  return (
    <div className="min-h-screen flex items-center justify-center bg-gray-900 p-8">
      <div className="w-full max-w-4xl">
        {/* Main plugin container */}
        <div className="bg-[#e8e8e8] rounded-sm shadow-2xl overflow-hidden" style={{
          backgroundImage: 'radial-gradient(circle, #999 1px, transparent 1px)',
          backgroundSize: '8px 8px'
        }}>
          {/* Header */}
          <div className="flex items-center justify-between px-6 py-4">
            <div className="tracking-wider" style={{ 
              fontSize: '32px', 
              fontWeight: 'bold',
              color: 'rgba(29, 161, 43, 1)',
              fontFamily: '"Koohinoor Gujarati", -apple-system, BlinkMacSystemFont, system-ui, sans-serif'
            }}>
              crit
            </div>
            
            <div className="flex items-center gap-4">
              <select
                value={preset}
                onChange={(e) => handleLoadPreset(e.target.value)}
                className="bg-white border border-gray-400 px-4 py-1 text-sm text-gray-700 rounded-sm"
                style={{
                  fontFamily: '"Koohinoor Gujarati", -apple-system, BlinkMacSystemFont, system-ui, sans-serif'
                }}
              >
                <option value="">Load preset</option>
                <option value="subtle-sizzle">Subtle Sizzle</option>
                <option value="aggressive-crunch">Aggressive Crunch</option>
                <option value="wide-spacey">Wide Spacey</option>
                <option value="distorted-pop">Distorted Pop</option>
              </select>
              
              <button
                onClick={handleSave}
                className="text-black px-6 py-1 tracking-wider transition-colors"
                style={{ 
                  fontWeight: 'bold',
                  backgroundColor: 'rgba(29, 161, 43, 1)',
                  fontFamily: '"Koohinoor Gujarati", -apple-system, BlinkMacSystemFont, system-ui, sans-serif'
                }}
                onMouseEnter={(e) => e.currentTarget.style.backgroundColor = 'rgba(24, 135, 36, 1)'}
                onMouseLeave={(e) => e.currentTarget.style.backgroundColor = 'rgba(29, 161, 43, 1)'}
              >
                SAVE
              </button>
            </div>
          </div>

          {/* Matrix Display */}
          <MatrixDisplay 
            pressure={pressure}
            fault={fault}
            split={split}
            momentum={momentum}
            resolve={resolve}
          />

          {/* Knobs Section */}
          <div className="bg-[#e8e8e8] px-8 py-12">
            <div className="grid grid-cols-6 gap-8">
              <RotaryKnob
                label="PRESSURE"
                sublabel="stress it out"
                value={pressure}
                onChange={setPressure}
              />
              <RotaryKnob
                label="FAULT"
                sublabel="break it"
                value={fault}
                onChange={setFault}
              />
              <RotaryKnob
                label="SPLIT"
                sublabel="pull it apart"
                value={split}
                onChange={setSplit}
              />
              <RotaryKnob
                label="MOMENTUM"
                sublabel="shape it"
                value={momentum}
                onChange={setMomentum}
              />
              <RotaryKnob
                label="RESOLVE"
                sublabel="rescue it"
                value={resolve}
                onChange={setResolve}
              />
              <RotaryKnob
                label="MIX"
                sublabel="blend"
                value={mix}
                onChange={setMix}
              />
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}