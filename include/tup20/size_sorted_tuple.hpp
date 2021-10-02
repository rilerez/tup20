#ifndef TUP20_SIZE_SORTED_TUPLE_HPP_INCLUDE_GUARD
#define TUP20_SIZE_SORTED_TUPLE_HPP_INCLUDE_GUARD

#include "impl/macros.hpp"

#include "tuple.hpp"
#include "permute.hpp"

#include <numeric>
#include <algorithm>

// the big question: how do you sort types in C++?
// Before constexpr array and algorithms, you would have to use a sort
// implemented in TMP. This means code duplication: essentially
// reimplementing chunks of std::algorithms in the pure functional turing
// tarpit that is old-school TMP.
//
// Now that we have constexpr std::sort, we should reuse it to sort types.
// But you can't make an array of types. So instead, let's sort on indices.
// But how do you order types/indices into types? By a type attribute: a
// function from types -> numbers. The particular attribute of interest is
// sizeof.
//
// How do we implement type attributes in C++? Typically with variable
// templates (or traditionally with class templates with a ::value field)
// But these are not first class---you can't define one in place (like a
// lambda expression). Instead, we'll use templated lambda expressions.
//
// For example,
// constexpr auto sizeof_attr = []<class T>(){return sizeof(T);}
// Then we can pass this as a template arg directly:
// higher_order_template<[]<class T>(){return sizeof(T);},....>

namespace tup20 {

/**
 * Creates a comparison function that compares the image of its inputs
 * under f. cmp_by(f,less)(x,y) => less(f(x),f(y))
 */
static constexpr auto cmp_by(auto f, auto less = std::less<>{}) {
  return [ f    = std::move(f),
           less = std::move(less) ] TUP20_FN(less(f(_0), f(_1)));
}

/**
 * Sort the indices as a stand-in for Ts via attribute and less.
 * Note: this is different from the permutation that sorts them (it's its
 * inverse)
 *
 * For example
 * attribute_sorting_indices<
 *     []<class T>() { return T::value; },
 *     std::less<>,
 *     std::integral_constant<int,3>, // index=0
 *     std::integral_constant<int,4>, // index=1
 *     std::integral_constant<int,2>, // index=2
 *     std::integral_constant<int,1>  // index=3
 * >
 * evaluates to std::array{3,2,0,1}
 */
template<auto attribute, class less, class... Ts>
inline constexpr auto attribute_sorting_indices = [] {
  constexpr auto       N{sizeof...(Ts)};
  constexpr std::array attrs{attribute.template operator()<Ts>()...};

  permutation<N> indices;
  std::iota(std::begin(indices), std::end(indices), 0u);
  std::sort(std::begin(indices),
            std::end(indices),
            cmp_by([=] TUP20_FN(attrs[_]), less {}));
  return indices;
}();

/**
 * The permutation that sorts Ts based on attribute and less.
 * Note: this is different from sorting indices. Instead it says where
 * that element goes after sorting.
 *
 * For example
 * attribute_sorting_indices<
 *     []<class T>() { return T::value; },
 *     std::less<>,
 *     std::integral_constant<int,3>, // index=0
 *     std::integral_constant<int,4>, // index=1
 *     std::integral_constant<int,2>, // index=2
 *     std::integral_constant<int,1>  // index=3
 * >
 * evaluates to permutation{2,3,1,0}
 *
 */
template<auto attribute, class less, class... Ts>
inline constexpr auto attribute_sorting_permutation =
    invert(attribute_sorting_indices<attribute, less, Ts...>);

template<class... Ts>
inline constexpr auto size_descending_permutation =
    attribute_sorting_permutation<[]<class T>() { return sizeof(T); },
                                  std::greater<>,
                                  Ts...>;

template<class... Ts>
struct size_sorted_tuple
    : storage_permuted_tuple<size_descending_permutation<Ts...>, Ts...> {
  using base =
      storage_permuted_tuple<size_descending_permutation<Ts...>, Ts...>;
  using base::base;
};
template<class... Ts>
size_sorted_tuple(Ts...) -> size_sorted_tuple<Ts...>;
} // namespace tup20
#endif // TUP20_SIZE_SORTED_TUPLE_HPP_INCLUDE_GUARD
