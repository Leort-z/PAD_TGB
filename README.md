# Fractal de Mandelbrot - Processamento de Alto Desempenho - Unicinos

Instalar SDL2 e MPI

compilar com "mpiCC mandelbrot.cpp -o mandelbrot -lSDL2"

executar com "mpirun -n 1 ./mandelbrot" onde n é o número de Threads

existem os seguintes parâmetros para o ./mandelbrot:
-s indica que ele vai dar um sleep de 10s nas threads trabalhadoras pares antes de enviar para ser exibido na tela o resultado

-w indica a largura da tela. Por padrão é 800.

-h indica a altura da tela. Por padrão é 800.

-i indica o número máximo de iterações no cálculo do fractal. Por padrão é 100.

Exemplo: mpirun -n 27 ./mandelbrot -s -w 1000 -h 500 -i 10

Será executada 25 threads trabalhadoras (0 é a mestre, 1 é a que exibe o fractal e 2-26 são as threads trabalhadoras), com o sleep de 10 segundos nas threads pares, com largura de 1000 e altura de 500 e com no máximo 10 iterações.
