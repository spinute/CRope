require 'Rope'

class ElasticRope
	attr_accessor :str

	def initialize(str='')
		raise ArgumentError unless str.is_a?(String)
		@str = str
	end

	def +(other)
		my_rope = @str.is_a?(String) ? Rope.new(@str) : @str
		other_rope = other.str.is_a?(String) ? Rope.new(other.str) : other.str
		@str = my_rope + other_rope
		self
	end

	def to_string
		@str = @str.to_s
	end
end
