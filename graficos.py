import matplotlib.pyplot as plt
import os 
import sys 
from pdb import set_trace as pause
import numpy as np
# 1. Defina seus dados do Eixo X (Categorias)
categorias = ['Conf 1a', 'Conf 1b', 'Conf 1c', 'FIXO', 'BASE']

# 2. Defina seus dados do Eixo Y (Substitua pelos seus valores reais)
# Valores aproximados extraídos da imagem:
dados_IPC={'ROB':{'fft': [0.8240,0.9169 ,0.9345 ,0.9336,0.9281],
              'astar': [0.7327,0.8486,0.8465,0.8463,0.8435],
              'kmeans': [0.7788,0.7758,0.7794,0.7773,0.8544]},
       'L1D':{'fft': [0.8976,0.9036, 0.9436,0.9336,0.9281],
              'astar': [0.8193,0.8402,0.8521,0.8463,0.8435],
              'kmeans': [0.7472,0.7451,0.7773,0.7773,0.8544]},
       'BP':{'fft': [0.9122, 0.9229,0.9544,0.9336,0.9281],
             'astar': [0.8467,0.8457,0.8491,0.8463,0.8435],
             'kmeans': [0.6338,0.7267,0.8139,0.7773,0.8544]}}


dados_TIME={'ROB':{'fft': [0.002452,0.002204,0.002162,0.002164,0.002177],
              'astar': [0.017682,0.015268,0.015306,0.015308,0.015360],
              'kmeans': [0.003151,0.003163,0.003148,0.003157,0.002872]},

       'L1D':{'fft': [0.002251,0.002236,0.002141,0.002164,0.002177],
              'astar': [0.015813,0.015420,0.015205,0.015308,0.015360],
              'kmeans': [0.003284,0.003293,0.003157,0.003157,0.002872]},

       'BP':{'fft': [0.002215,0.002189,0.002117,0.002164,0.002177],
             'astar': [0.015302,0.015319,0.015258,0.015308,0.015360],
             'kmeans': [0.003871,0.003376,0.003015,0.003157,0.002872]}}


pasta_saida = 'graficos_pdf'
pasta_saida_tempo = 'graficos_tempo'
os.makedirs(pasta_saida, exist_ok=True)
os.makedirs(pasta_saida_tempo, exist_ok=True) # <-- CORREÇÃO: Criar pasta de tempo
# 3. Configuração da figura
for i in dados_IPC:
    fig, ax = plt.subplots(figsize=(8, 5))

    ax.plot(categorias, 1000*dados_IPC[i]['fft'], label='fft', color='#9BBB59', linewidth=2.5,marker='o', markersize=6)
    ax.plot(categorias, 1000*dados_IPC[i]['astar'], label='astar', color='#C0504D', linewidth=2.5,marker='o', markersize=6)
    ax.plot(categorias, 1000*dados_IPC[i]['kmeans'], label='kmeans', color='#4F81BD', linewidth=2.5,marker='o', markersize=6)

    ax.set_ylim(0.5, 1.0)
    # np.arange(início, fim, passo). Colocamos 1.6 como fim para garantir que o 1.5 seja incluído.
    ax.set_yticks(np.arange(0.5, 1.1, 0.1))

    # 6. Adicionar as linhas de grade horizontais
    ax.grid(axis='y', color='gray', linestyle='-', linewidth=0.5, alpha=0.7)

    # 7. Posicionar a legenda fora do gráfico (à direita)
    ax.legend(loc='center left', bbox_to_anchor=(1.02, 0.5), frameon=False, prop={'size': 12})

    # 8. Limpeza do visual (Remover bordas superior e direita para ficar idêntico)
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['left'].set_color('gray')
    ax.spines['bottom'].set_color('gray')

    ax.set_ylabel(f'IPC - {i}')

    # Ajusta o layout para garantir que a legenda não fique cortada
    plt.tight_layout()

    # 9. Define o caminho para salvar o arquivo PDF
    caminho_arquivo = os.path.join(pasta_saida, f'grafico_{i}.pdf')
    
    # Salva o gráfico
    plt.savefig(caminho_arquivo, format='pdf', bbox_inches='tight')
    
    # IMPORTANTE: Fecha a figura atual para limpar a memória antes do próximo loop
    plt.close(fig)

print(f"Gráficos salvos com sucesso na pasta _ IPC '{pasta_saida}'!")


for i in dados_TIME:
    fig, ax = plt.subplots(figsize=(8, 5))

    # CORREÇÃO: Usando np.array() para multiplicar os valores da lista por 1000 corretamente
    ax.plot(categorias, np.array(dados_TIME[i]['fft']) * 1000, label='fft', color='#9BBB59', linewidth=2.5,marker='o', markersize=6)
    ax.plot(categorias, np.array(dados_TIME[i]['astar']) * 1000, label='astar', color='#C0504D', linewidth=2.5,marker='s', markersize=6)
    ax.plot(categorias, np.array(dados_TIME[i]['kmeans']) * 1000, label='kmeans', color='#4F81BD', linewidth=2.5,marker='x', markersize=6)

    # CORREÇÃO: Ajustado o limite Y para até 20.0, pois o Astar chega a ~17.6 ms
    ax.set_ylim(0.0, 20.0)
    ax.set_yticks(np.arange(0.0, 22.0, 2.0))

    ax.grid(axis='y', color='gray', linestyle='-', linewidth=0.5, alpha=0.7)
    ax.legend(loc='center left', bbox_to_anchor=(1.02, 0.5), frameon=False, prop={'size': 12})

    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['left'].set_color('gray')
    ax.spines['bottom'].set_color('gray')

    ax.set_ylabel(f'TEMPO - {i} (ms)')

    plt.tight_layout()
    caminho_arquivo = os.path.join(pasta_saida_tempo, f'grafico_{i}.pdf')
    plt.savefig(caminho_arquivo, format='pdf', bbox_inches='tight')
    plt.close(fig)

print(f"Gráficos salvos com sucesso na pasta _ TEMPO EXE '{pasta_saida_tempo}'!")