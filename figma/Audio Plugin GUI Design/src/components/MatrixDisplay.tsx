import { useEffect, useRef } from 'react';

interface MatrixDisplayProps {
  pressure: number;
  fault: number;
  split: number;
  momentum: number;
  resolve: number;
}

export function MatrixDisplay({ pressure, fault, split, momentum, resolve }: MatrixDisplayProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const animationRef = useRef<number>();
  const timeRef = useRef(0);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const width = canvas.width;
    const height = canvas.height;

    const gridSize = 8;
    const cols = Math.floor(width / gridSize);
    const rows = Math.floor(height / gridSize);

    // Create a grid of cells
    const cells: boolean[][] = Array(rows).fill(null).map(() => 
      Array(cols).fill(false)
    );

    // Initialize with a pattern
    for (let row = 0; row < rows; row++) {
      for (let col = 0; col < cols; col++) {
        cells[row][col] = Math.random() > 0.5;
      }
    }

    const animate = () => {
      timeRef.current += 0.016; // ~60fps

      // Calculate stress level from parameters
      const stress = (pressure / 100) * (1 + fault / 200);
      const instability = (fault / 100) * (pressure / 100);
      const divergence = split / 100;
      const speed = 0.5 + (momentum / 100) * 2;
      const stability = resolve / 100;

      // Clear canvas
      ctx.fillStyle = '#000000';
      ctx.fillRect(0, 0, width, height);

      // Update cells based on stress
      if (Math.random() < 0.1 + instability * 0.3) {
        const randomRow = Math.floor(Math.random() * rows);
        const randomCol = Math.floor(Math.random() * cols);
        cells[randomRow][randomCol] = !cells[randomRow][randomCol];
      }

      // Draw grid
      for (let row = 0; row < rows; row++) {
        for (let col = 0; col < cols; col++) {
          if (cells[row][col]) {
            // Add some variation based on parameters
            let alpha = 1.0;
            
            // Fault creates flickering
            if (instability > 0.1) {
              alpha *= 0.7 + Math.random() * 0.3 * instability;
            }

            // Resolve stabilizes
            alpha *= (1 - stability * 0.3);

            // Split creates horizontal offset
            const offsetX = divergence * Math.sin(timeRef.current * speed + row * 0.5) * 2;

            const greenValue = Math.floor(200 + Math.random() * 55);
            ctx.fillStyle = `rgba(29, 161, 43, ${alpha})`;
            
            const x = col * gridSize + offsetX;
            const y = row * gridSize;
            
            // Add glow effect when pressure is high
            if (stress > 0.5) {
              ctx.shadowBlur = 2 + stress * 4;
              ctx.shadowColor = 'rgba(29, 161, 43, 0.8)';
            } else {
              ctx.shadowBlur = 0;
            }

            ctx.fillRect(x, y, gridSize - 1, gridSize - 1);
          }
        }
      }

      // Add scan lines for that retro feel
      ctx.shadowBlur = 0;
      ctx.fillStyle = 'rgba(0, 0, 0, 0.1)';
      for (let i = 0; i < height; i += 2) {
        ctx.fillRect(0, i, width, 1);
      }

      // Add noise/grain
      const imageData = ctx.getImageData(0, 0, width, height);
      const data = imageData.data;
      const noiseAmount = instability * 20;
      
      for (let i = 0; i < data.length; i += 4) {
        if (Math.random() < 0.02) {
          const noise = (Math.random() - 0.5) * noiseAmount;
          data[i + 1] = Math.max(0, Math.min(255, data[i + 1] + noise)); // Green channel
        }
      }
      
      ctx.putImageData(imageData, 0, 0);

      animationRef.current = requestAnimationFrame(animate);
    };

    animate();

    return () => {
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
    };
  }, [pressure, fault, split, momentum, resolve]);

  return (
    <div className="bg-black mx-6 my-4" style={{ 
      boxShadow: 'inset 0 4px 12px rgba(0,0,0,0.8)' 
    }}>
      <canvas
        ref={canvasRef}
        width={800}
        height={240}
        className="w-full h-auto"
        style={{ display: 'block' }}
      />
    </div>
  );
}