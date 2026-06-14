import numpy as np
import subprocess
import sys

def compile_c_code():
    print("Executando Make para compilação estrita do binário C...")
    subprocess.run(["make", "clean"], stdout=subprocess.DEVNULL)
    compilation = subprocess.run(["make"], capture_output=True, text=True)
    if compilation.returncode != 0:
        print("[-] Erro fatal durante a compilação do código C:\n", compilation.stderr)
        sys.exit(1)
    print("[+] Binário 'fft_test' gerado com sucesso via Makefile.\n")

def run_c_fft(signal):
    N = len(signal)
    # Codifica o vetor de números complexos em formato de string textual (real imag\n)
    input_str = "\n".join([f"{c.real} {c.imag}" for c in signal])
    
    # Dispara o processo passando o tamanho N DIRETAMENTE via argumentos de linha de comando
    process = subprocess.run(
        ["./fft_test", str(N)], 
        input=input_str, 
        capture_output=True, 
        text=True
    )
    
    if process.returncode != 0:
        print(f"[-] Ocorreu um erro interno na execução do binário C:\n{process.stderr}")
        sys.exit(1)

    # Coleta a saída do stdout do C e reconverte para array numérico complexo
    lines = process.stdout.strip().split('\n')
    c_results = np.zeros(N, dtype=np.complex128)
    for i, line in enumerate(lines):
        if line:
            r, im = map(float, line.split())
            c_results[i] = r + im * 1j
    return c_results

def test_dataset(name, signal):
    print(f"Analisando Dataset: {name} (Dimensão N = {len(signal)})")
    
    # Calcula a FFT padrão-ouro usando o algoritmo altamente vetorizado do NumPy (FFTW/MKL backend)
    expected_fft = np.fft.fft(signal)
    
    # Invoca a nossa rotina C compilada com passagem de parâmetros por linha de comando
    computed_fft = run_c_fft(signal)
    
    # Avalia o erro absoluto máximo ponto a ponto (Checagem de consistência aritmética)
    max_absolute_error = np.max(np.abs(expected_fft - computed_fft))
    
    # Tolerância rigorosa padrão IEEE 754 de dupla precisão de 1e-9
    if max_absolute_error < 1e-9:
        print(f"  [SUCESSO] Erro Máximo Absoluto Verificado: {max_absolute_error:.2e}")
    else:
        print(f"  [FALHA] Discrepância matemática detectada! Erro Máximo: {max_absolute_error:.2e}")

if __name__ == "__main__":
    compile_c_code()
    
    # Instanciação estruturada de 5 datasets com características de comportamento e tamanhos diferentes
    datasets = {}

    # Dataset 1: Impulso Unitário (Delta de Dirac) - Tamanho ultra compacto N=8
    # Comportamento matemático esperado: Espectro plano e constante em todas as frequências.
    N1 = 8
    ds1 = np.zeros(N1, dtype=complex)
    ds1[0] = 1.0 + 0j
    datasets["1. Impulso Isolado (Delta de Dirac)"] = ds1

    # Dataset 2: Componente Puramente DC (Sinal Constante) - Tamanho N=32
    # Comportamento matemático esperado: Toda a energia concentrada estritamente na frequência zero (índice 0).
    N2 = 32
    datasets["2. Nível Contínuo (Componente DC Estática)"] = np.ones(N2, dtype=complex)

    # Dataset 3: Senoide Harmônica Pura - Tamanho N=128
    # Comportamento matemático esperado: Dois picos nítidos e simétricos correspondentes à frequência fundamental.
    N3 = 128
    t3 = np.linspace(0, 1, N3, endpoint=False)
    datasets["3. Senoide Pura Fundamental (20 Hz)"] = np.sin(2 * np.pi * 20 * t3) + 0j

    # Dataset 4: Sinal Composto por Múltiplas Frequências e Fase Amortecida - Tamanho N=512
    # Comportamento matemático esperado: Múltiplas raias espectrais discretas perfeitamente isoladas.
    N4 = 512
    t4 = np.linspace(0, 1, N4, endpoint=False)
    datasets["4. Sinal Harmônico Composto (45Hz + 120Hz)"] = (
        np.sin(2 * np.pi * 45 * t4) + 0.7 * np.cos(2 * np.pi * 120 * t4) + 0j
    )

    # Dataset 5: Ruído de Alta Densidade Estocástica (Stress Test de Cache) - Tamanho Elevado N=4096
    # Comportamento matemático esperado: Espectro ruidoso distribuído aleatoriamente. 
    # Força a execução contínua de saltos de stride massivos na hierarquia de cache L2/L3.
    N5 = 4096
    np.random.seed(123)
    datasets["5. Ruído Branco Gaussiano Complexo Estocástico"] = (
        np.random.randn(N5) + 1j * np.random.randn(N5)
    )

    print("=" * 65)
    print("INICIANDO SUÍTE DE TESTES CROSS-VALIDADA C / NUMPY")
    print("=" * 65)
    for name, signal_data in datasets.items():
        test_dataset(name, signal_data)
    print("=" * 65)
    print("Bateria de testes encerrada.")
