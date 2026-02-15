apt update

# 容器配置
apt install -y git vim gdbserver

# 构建工具
apt install -y build-essential cmake

# quiet项目依赖
apt install -y libliquid-dev libfec-dev libjansson-dev libsndfile1-dev portaudio19-dev

# quiet-lwip项目依赖
apt install -y liblwip-dev

# 运行时需要的配置文件
mkdir /usr/local/share/quiet
cp /home/ubuntu/quiet/quiet-profiles.json /usr/local/share/quiet/quiet-profiles.json
