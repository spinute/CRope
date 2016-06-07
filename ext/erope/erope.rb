require 'Rope'

class ElasticRope
	def initialize
		@is_rope = false
		@str = ""
	end

	def +(other)
		my_rope = @is_rope ? self : Rope.new(@str)
		@is_rope = true
		@str = my_rope + other.is_s? ? Rope.new(other) : other
	end

	def to_string
		@is_rope = false
		@str = @str.to_s
	end
end
