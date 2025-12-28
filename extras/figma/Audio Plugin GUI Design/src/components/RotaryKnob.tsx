import { useState, useRef, useEffect } from 'react';

interface RotaryKnobProps {
  label: string;
  sublabel: string;
  value: number;
  onChange: (value: number) => void;
  min?: number;
  max?: number;
}

export function RotaryKnob({ 
  label, 
  sublabel, 
  value, 
  onChange, 
  min = 0, 
  max = 100 
}: RotaryKnobProps) {
  const [isDragging, setIsDragging] = useState(false);
  const knobRef = useRef<HTMLDivElement>(null);
  const startYRef = useRef(0);
  const startValueRef = useRef(0);

  const handleMouseDown = (e: React.MouseEvent) => {
    setIsDragging(true);
    startYRef.current = e.clientY;
    startValueRef.current = value;
  };

  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (!isDragging) return;

      const deltaY = startYRef.current - e.clientY;
      const sensitivity = 0.5;
      const newValue = Math.max(min, Math.min(max, startValueRef.current + deltaY * sensitivity));
      onChange(Math.round(newValue));
    };

    const handleMouseUp = () => {
      setIsDragging(false);
    };

    if (isDragging) {
      document.addEventListener('mousemove', handleMouseMove);
      document.addEventListener('mouseup', handleMouseUp);
    }

    return () => {
      document.removeEventListener('mousemove', handleMouseMove);
      document.removeEventListener('mouseup', handleMouseUp);
    };
  }, [isDragging, min, max, onChange, value]);

  // Calculate rotation angle (-135 to 135 degrees)
  const percentage = (value - min) / (max - min);
  const rotation = -135 + percentage * 270;

  return (
    <div className="flex flex-col items-center gap-3">
      {/* Knob */}
      <div
        ref={knobRef}
        onMouseDown={handleMouseDown}
        className="relative cursor-pointer select-none"
        style={{ width: '80px', height: '80px' }}
      >
        {/* Outer circle */}
        <div className="absolute inset-0 bg-black rounded-full shadow-lg" />
        
        {/* Green fill rim - fills from left to right */}
        <svg className="absolute inset-0 w-full h-full rotate-180" style={{ overflow: 'visible' }}>
          <circle
            cx="40"
            cy="40"
            r="38"
            fill="none"
            stroke="rgba(29, 161, 43, 1)"
            strokeWidth="4"
            strokeDasharray={`${percentage * 270 * (Math.PI * 76 / 360)} ${Math.PI * 76}`}
            strokeLinecap="round"
            style={{
              filter: 'drop-shadow(0 0 4px rgba(29, 161, 43, 0.6))'
            }}
          />
        </svg>
        
        {/* Inner circle with gradient */}
        <div 
          className="absolute inset-2 bg-gradient-to-br from-gray-800 to-black rounded-full"
          style={{
            boxShadow: 'inset 0 2px 8px rgba(0,0,0,0.8), inset 0 -2px 4px rgba(255,255,255,0.1)'
          }}
        />

        {/* Center cap */}
        <div className="absolute inset-0 flex items-center justify-center">
          <div className="w-4 h-4 bg-gradient-to-br from-gray-700 to-gray-900 rounded-full" 
               style={{ boxShadow: '0 1px 3px rgba(0,0,0,0.5)' }} />
        </div>
      </div>

      {/* Label */}
      <div className="flex flex-col items-center gap-1">
        <div className="bg-black px-3 py-1">
          <span className="tracking-wider" style={{ 
            fontSize: '11px', 
            fontWeight: 'bold',
            color: 'rgba(29, 161, 43, 1)',
            fontFamily: '"Koohinoor Gujarati", -apple-system, BlinkMacSystemFont, system-ui, sans-serif'
          }}>
            {label}
          </span>
        </div>
        <div className="bg-black px-2 py-0.5">
          <span style={{ 
            fontSize: '8px',
            color: 'rgba(29, 161, 43, 1)',
            fontFamily: '"Koohinoor Gujarati", -apple-system, BlinkMacSystemFont, system-ui, sans-serif'
          }}>
            {Math.round(value)}%
          </span>
        </div>
      </div>
    </div>
  );
}