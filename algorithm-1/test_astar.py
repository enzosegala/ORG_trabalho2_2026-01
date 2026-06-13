import subprocess
import sys
import random

# Configurações de dimensão comuns para a bateria de testes
W, H = 20, 20
START = (0, 0)
GOAL = (19, 19)

def generate_datasets():
    datasets = {}

    # Dataset 1: Grid Totalmente Aberto (Cenário de melhor caso para propagação)
    datasets["1. Grid Totalmente Aberto"] = [0] * (W * H)

    # Dataset 2: Parede Divisória Central (Força o contorno completo e expansão lateral)
    grid_wall = [0] * (W * H)
    for y in range(4, 16): 
        grid_wall[y * W + 10] = 1
    datasets["2. Parede Divisoria Central"] = grid_wall

    # Dataset 3: Labirinto Simples em Formato de Serpentina (Zig-Zag)
    grid_zigzag = [0] * (W * H)
    for x in range(0, 16): grid_zigzag[5 * W + x] = 1
    for x in range(4, 20): grid_zigzag[11 * W + x] = 1
    for x in range(0, 16): grid_zigzag[16 * W + x] = 1
    datasets["3. Labirinto em Serpentina"] = grid_zigzag

    # Dataset 4: Ruído Esparso Aleatório (Gera micro-obstáculos, estressa decisões locais)
    random.seed(101)  # Fixa a seed para reprodutibilidade dos testes de hardware
    grid_sparse = [0] * (W * H)
    for i in range(W * H):
        if random.random() > 0.73 and (i % W, i // W) != START and (i % W, i // W) != GOAL:
            grid_sparse[i] = 1
    datasets["4. Obstaculos Esparsos Aleatorios"] = grid_sparse

    # Dataset 5: Cenário sem Solução Possível (Muro fechando completamente o Objetivo)
    grid_impos = [0] * (W * H)
    grid_impos[18 * W + 19] = 1
    grid_impos[19 * W + 18] = 1
    grid_impos[18 * W + 18] = 1
    datasets["5. Destino Bloqueado (Impossivel)"] = grid_impos

    return datasets

def execute_binary(grid, h_type):
    # Formata a string contendo a matriz plana para o stdin do executável C
    grid_input = " ".join(map(str, grid))
    
    # Monta os argumentos de linha de comando: ./astar <w> <h> <sx> <sy> <gx> <gy> <h_type>
    cmd = [
        "./astar", 
        str(W), str(H), 
        str(START[0]), str(START[1]), 
        str(GOAL[0]), str(GOAL[1]), 
        str(h_type)
    ]
    
    try:
        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        stdout, stderr = proc.communicate(input=grid_input)
        if proc.returncode != 0:
            return f"Erro de Execução (Code {proc.returncode}): {stderr.strip()}"
        return stdout.strip()
    except FileNotFoundError:
        return "Erro: O executável './astar' não foi encontrado. Execute 'make' primeiro."

def main():
    print("=" * 70)
    print("BATERIA DE VERIFICAÇÃO DO ALGORITMO A* (VALIDAÇÃO DE DADOS E HEURÍSTICAS)")
    print("=" * 70)
    
    datasets = generate_datasets()
    
    for name, grid in datasets.items():
        print(f"\n➔ Conjunto de Dados: {name}")
        print("-" * 55)
        
        # Teste com Heurística 1 (Manhattan)
        res_manhattan = execute_binary(grid, 1)
        print(f"  [Heurística Manhattan]  -> {res_manhattan}")
        
        # Teste com Heurística 2 (Euclidiana)
        res_euclidean = execute_binary(grid, 2)
        print(f"  [Heurística Euclidiana] -> {res_euclidean}")

if __name__ == "__main__":
    main()
