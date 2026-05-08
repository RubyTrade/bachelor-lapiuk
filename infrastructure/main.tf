provider "aws" {
  region = "us-east-1"
}

data "aws_ami" "ubuntu" {
  most_recent = true
  owners      = ["099720109477"] # Canonical
  filter {
    name   = "name"
    values = ["ubuntu/images/hvm-ssd/ubuntu-jammy-22.04-amd64-server-*"]
  }
}

data "aws_security_group" "kafka_ec2_sg" {
  filter {
    name   = "group-name"
    values = ["rubytrade-kafka-ec2-sg"]
  }
}

resource "aws_key_pair" "my_key" {
  key_name   = "kafka-ec2"
  public_key = file("~/.ssh/kafka_ec2.pub")
}

# resource "aws_instance" "ec2" {
#  ami = data.aws_ami.ubuntu.id   
#  instance_type = "t3.small"

#  key_name      = aws_key_pair.my_key.key_name
#  vpc_security_group_ids = [data.aws_security_group.kafka_ec2_sg.id]

#  tags = {
#   Name = "terraform-ec2"
#  }
#}

#output "public_ip" {
#  value = aws_instance.ec2.public_ip
#}
